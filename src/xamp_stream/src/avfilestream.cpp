#include <stream/avfilestream.h>
#include <stream/avlib.h>

#include <base/assert.h>
#include <base/exception.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/scopeguard.h>
#include <base/math.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(AvFileStream);

class AvFileStream::AvFileStreamImpl {
public:
    /*
    * Constructor.
    * 
    */
    AvFileStreamImpl()
        : audio_stream_id_(-1)
        , duration_(0.0) {
        logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(AvFileStream));
    }

    /*
    * Destructor.
    * 
    */
    ~AvFileStreamImpl() noexcept {
        Close();
    }

    /*
    * Close file stream.
    * 
    */
    void Close() noexcept {
        audio_stream_id_ = -1;
        duration_ = 0;
        audio_format_.Reset();
        swr_context_.reset();
        audio_frame_.reset();
        if (codec_context_ != nullptr) {
            LIBAV_LIB.Codec->avcodec_close(codec_context_.get());
            codec_context_ = nullptr;
        }
        format_context_.reset();
    }

    OrderedMap<std::wstring, std::wstring> ProbeFileInfo(const Path& file_path) {
        AVFormatContext* ctx = nullptr;
        AvPtr<AVFormatContext> format_context;
        AVDictionary* options = nullptr;

        const auto file_path_ut8 = String::ToString(file_path.wstring());
        const auto err = LIBAV_LIB.Format->avformat_open_input(&ctx, file_path_ut8.c_str(), nullptr, &options);
        if (err != 0) {
            return {};
        }

        format_context.reset(ctx);

        OrderedMap<std::wstring, std::wstring> file_info;
        AVDictionaryEntry* tag = nullptr;
        while ((tag = LIBAV_LIB.Util->av_dict_get(format_context->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
            file_info[String::ToString(tag->key)] = String::ToString(tag->value);
            XAMP_LOG_D(logger_, "{} {}", tag->key, tag->value);
        }
        return file_info;
    }

    /*
    * Load file from file path.
    * 
    * @param file_path File path.
    */
    void OpenFile(const Path & file_path) {
        ProbeFileInfo(file_path);

        AVFormatContext* format_context = nullptr;
        AVDictionary* options = nullptr;
        
        if (!IsFilePath(file_path)) {
            // note: Http request timeout in microseconds. 
            LIBAV_LIB.Util->av_dict_set(&options, "timeout", "6000000", 0);
            LIBAV_LIB.Util->av_dict_set(&options, "user_agent", XAMP_HTTP_USER_AGENT, 0);
        }
        LIBAV_LIB.Util->av_dict_set(&options, "probesize", "4M", AV_OPT_SEARCH_CHILDREN);

        const auto file_path_ut8 = String::ToString(file_path.wstring());
        const auto err = LIBAV_LIB.Format->avformat_open_input(&format_context, file_path_ut8.c_str(), nullptr, &options);
        if (err != 0) {
            static constexpr auto AVERROR_NOFMT = -42;
            switch (err) {
            case AVERROR_INVALIDDATA:
                throw NotSupportFormatException();
                break;
            case AVERROR_NOFMT:
                throw FileNotFoundException();
            default:
                throw AvException(err);
            }
        }

        if (format_context != nullptr) {
            format_context_.reset(format_context);
        }
        else {
            throw NotSupportFormatException();
        }

        // max analyze duration: 5s
        format_context->max_analyze_duration = 5 * AV_TIME_BASE;

        if (LIBAV_LIB.Format->avformat_find_stream_info(format_context_.get(), nullptr) < 0) {
            throw NotSupportFormatException();
        }

        for (uint32_t i = 0; i < format_context_->nb_streams; ++i) {
            if ((audio_stream_id_ < 0) && (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)) {
                audio_stream_id_ = i;
                codecpar_ = format_context_->streams[i]->codecpar;
            }
        }

        if (HasAudio()) {
            OpenAudioStream();
        }
        else {
            throw NotSupportFormatException();
        }

        const auto stream = format_context_->streams[audio_stream_id_];
        // todo: opus格式可能會讀不出duration.
        if (stream->duration == AV_NOPTS_VALUE) {
            duration_ = 0;
            return;
        }
        duration_ = ::av_q2d(stream->time_base) * static_cast<double>(stream->duration);
    }

    /*
    * Open audio stream.
    * 
    * @throw NotSupportFormatException
    */
    void OpenAudioStream() {
        const AVCodec* codec = nullptr;

        if (!codec) {
            codec = LIBAV_LIB.Codec->avcodec_find_decoder(codecpar_->codec_id);
            if (!codec) {
                throw NotSupportFormatException();
            }
        }

        auto codec_context = LIBAV_LIB.Codec->avcodec_alloc_context3(codec);
        if (!codec_context) {
            throw NotSupportFormatException();
        }
        codec_context_.reset(codec_context);

        AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_parameters_to_context(codec_context_.get(), codecpar_));
        AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_open2(codec_context_.get(), codec, nullptr));
        
        audio_frame_.reset(LIBAV_LIB.Util->av_frame_alloc());

        switch (codec_context_->sample_fmt) {
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S32:
            XAMP_LOG_D(logger_, "Stream format = > INTERLEAVED");
            break;
        case AV_SAMPLE_FMT_S16P:
        case AV_SAMPLE_FMT_S32P:
        case AV_SAMPLE_FMT_FLTP:
            XAMP_LOG_D(logger_, "Stream format => DEINTERLEAVED");
            break;
        default:
            throw NotSupportFormatException();
        }

        auto FormatBitRate = [this]() {
            std::ostringstream ostr;
            if (GetBitRate() > 1000.0) {
                ostr << Round(GetBitRate() / 1000.0, 1) << " Kbps";
            } else { 
                ostr << GetBitRate() << " bps";
            }
            return ostr.str();
            };

        if (format_context_->iformat != nullptr) {
            XAMP_LOG_D(logger_, "Stream input format => {} bitdetph:{} bitrate:{}",
                format_context_->iformat->name,
                GetBitDepth(),
                FormatBitRate());
        }

        const int64_t channel_layout = codec_context_->channel_layout == 0
    	? AV_CH_LAYOUT_STEREO : static_cast<int64_t>(codec_context_->channel_layout);
       
        swr_context_.reset(LIBAV_LIB.Swr->swr_alloc_set_opts(swr_context_.get(),
            AV_CH_LAYOUT_STEREO,
            AV_SAMPLE_FMT_FLT,
            codec_context_->sample_rate,
            channel_layout,
            codec_context_->sample_fmt,
            codec_context_->sample_rate,
            0,
            nullptr));

        // Down mix to stereo.
        AvIfFailedThrow(LIBAV_LIB.Swr->swr_init(swr_context_.get()));
        audio_format_.SetFormat(DataFormat::FORMAT_PCM);
        if (codec_context_->channels != AudioFormat::kMaxChannel) {
            XAMP_LOG_D(logger_,"Mix {} to Stereo channel", codec_context_->channels);
            audio_format_.SetChannel(AudioFormat::kMaxChannel);
        } else {
            audio_format_.SetChannel(static_cast<uint16_t>(codec_context_->channels));
        }
        audio_format_.SetSampleRate(static_cast<uint32_t>(codec_context_->sample_rate));
        audio_format_.SetBitPerSample(LIBAV_LIB.Util->av_get_bytes_per_sample(codec_context_->sample_fmt) * 8);
        audio_format_.SetPackedFormat(PackedFormat::INTERLEAVED);
        XAMP_LOG_D(logger_, "Stream format: {}", audio_format_);

        packet_.reset(LIBAV_LIB.Codec->av_packet_alloc());
        LIBAV_LIB.Codec->av_init_packet(packet_.get());
        is_eof_ = false;
    }

    /*
    * Get audio samples.
    * 
    * @param buffer The buffer to store samples.
    * @param length The length of buffer.
    * 
    * @return The number of samples read.
    */
    uint32_t GetSamples(float* buffer, uint32_t length) const {
        const auto channel_read_samples = length / sizeof(float);
        uint32_t num_read_sample = 0;

        for (uint32_t i = 0; i < format_context_->nb_streams; ++i) {
            int ret = 0;
            while (ret >= 0) {
                ret = LIBAV_LIB.Format->av_read_frame(format_context_.get(), packet_.get());
                if (ret == AVERROR_EOF) {
                    is_eof_ = true;
                    return 0;
                }

                XAMP_ON_SCOPE_EXIT(LIBAV_LIB.Codec->av_packet_unref(packet_.get()));

                if (packet_->stream_index != audio_stream_id_) {
                    continue;
                }

                auto ret = LIBAV_LIB.Codec->avcodec_send_packet(codec_context_.get(), packet_.get());
                if (ret == AVERROR(EAGAIN)) {
                    break;
                }

                if (ret == AVERROR_EOF || ret == AVERROR(EINVAL)) {
                    return num_read_sample;
                }

                if (ret == AVERROR_INVALIDDATA) {
                    AvIfFailedThrow(ret);
                }

                while (ret >= 0) {
                    XAMP_ON_SCOPE_EXIT(LIBAV_LIB.Util->av_frame_unref(audio_frame_.get()));

                    ret = LIBAV_LIB.Codec->avcodec_receive_frame(codec_context_.get(), audio_frame_.get());
                    if (ret == AVERROR(EAGAIN)) {
                        break;
                    }

                    if (ret == AVERROR_EOF || ret == AVERROR(EINVAL)) {
                        return num_read_sample;
                    }

                    const auto convert_size = ConvertSamples(buffer + num_read_sample, length - num_read_sample);
                    if (convert_size == 0) {
                        return 0;
                    }
                    num_read_sample += convert_size;
                    if (num_read_sample >= channel_read_samples) {
                        return num_read_sample;
                    }
                }
            }
        }
        return num_read_sample;
    }

    AudioFormat GetFormat() const noexcept {
        return audio_format_;
    }

    double GetDuration() const noexcept {
        return duration_;
    }

    uint32_t GetSampleSize() const noexcept {
        return sizeof(float);
    }

    int64_t GetBitRate() const noexcept {
        return format_context_->streams[audio_stream_id_]->codecpar->bit_rate;
    }

    uint32_t GetBitDepth() const noexcept {
        return (std::max)(format_context_->streams[audio_stream_id_]->codecpar->bits_per_raw_sample, 16);
    }

    void Seek(double stream_time) const {
        XAMP_EXPECTS(stream_time >= 0);

        if (codec_context_ == nullptr) {
            return;
        }

        auto timestamp = static_cast<int64_t>(stream_time * AV_TIME_BASE);

        if (format_context_->start_time != AV_NOPTS_VALUE) {
            timestamp += format_context_->start_time;
        }

        AvIfFailedThrow(LIBAV_LIB.Format->av_seek_frame(format_context_.get(), -1, timestamp, AVSEEK_FLAG_BACKWARD));
        LIBAV_LIB.Codec->avcodec_flush_buffers(codec_context_.get());
        is_eof_ = false;
    }

    /*
    * Check if the stream is active.
    * 
    * @return True if the stream is active.
    */
    bool IsActive() const {
        return !is_eof_;
    }
private:
    /*
    * Check if the stream has audio.
    * 
    * @return True if the stream has audio.
    */
    XAMP_NO_DISCARD bool HasAudio() const noexcept {
        return audio_stream_id_ >= 0;
    }

    /*
    * Convert audio samples.
    * 
    * @param buffer The buffer to store samples.
    * @param length The length of buffer.
    * 
    * @return The number of samples converted.
    */
    uint32_t ConvertSamples(float* buffer, uint32_t length) const noexcept {
        if (audio_frame_->nb_samples > length) {
            XAMP_LOG_ERROR("Convert sample failure! read buffer too small {} < {}", 
                length, audio_frame_->nb_samples);
            return 0;
        }
        // 輸出Channel都是立體聲, 所以要將nb_samples * 2
        const int32_t frame_size = audio_frame_->nb_samples * AudioFormat::kMaxChannel;
        const auto result = LIBAV_LIB.Swr->swr_convert(swr_context_.get(),
            reinterpret_cast<uint8_t**>(&buffer),
            frame_size,
            const_cast<const uint8_t**>(audio_frame_->data),
            audio_frame_->nb_samples);
        if (result <= 0) {
            XAMP_LOG_ERROR("swr_convert return failure({})!", result);
            return 0;
        }
        return frame_size;
    }

    mutable bool is_eof_{ false };
    int32_t audio_stream_id_;
    double duration_;
    AudioFormat audio_format_;
    AvPtr<SwrContext> swr_context_;
    AvPtr<AVFrame> audio_frame_;
    AvPtr<AVCodecContext> codec_context_{nullptr};
    AVCodecParameters *codecpar_{nullptr};
    AvPtr<AVFormatContext> format_context_;
    AvPtr<AVPacket> packet_;
    LoggerPtr logger_;
};


AvFileStream::AvFileStream()
    : impl_(MakeAlign<AvFileStreamImpl>()) {
}

XAMP_PIMPL_IMPL(AvFileStream)

void AvFileStream::OpenFile(const Path & file_path) {
    return impl_->OpenFile(file_path);
}

void AvFileStream::Close() noexcept {
    impl_->Close();
}

double AvFileStream::GetDurationAsSeconds() const {
    return impl_->GetDuration();
}

AudioFormat AvFileStream::GetFormat() const noexcept {
    return impl_->GetFormat();
}

uint32_t AvFileStream::GetSamples(void* buffer, uint32_t length) const {
    return impl_->GetSamples(static_cast<float *>(buffer), length);
}

void AvFileStream::SeekAsSeconds(double stream_time) const {
    impl_->Seek(stream_time);
}

std::string_view AvFileStream::GetDescription() const noexcept {
    return Description;
}

uint32_t AvFileStream::GetSampleSize() const noexcept {
    return impl_->GetSampleSize();
}

bool AvFileStream::IsActive() const noexcept {
    return impl_->IsActive();
}

uint32_t AvFileStream::GetBitDepth() const {
    return impl_->GetBitDepth();
}

int64_t AvFileStream::GetBitRate() const {
    return impl_->GetBitRate();
}

Uuid AvFileStream::GetTypeId() const {
    return XAMP_UUID_OF(AvFileStream);
}

XAMP_STREAM_NAMESPACE_END
