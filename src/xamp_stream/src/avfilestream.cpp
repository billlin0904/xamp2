#include <base/exception.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/scopeguard.h>
#include <base/math.h>

#include <stream/avlib.h>
#include <stream/avfilestream.h>

namespace xamp::stream {

XAMP_DECLARE_LOG_NAME(AvFileStream);

class XAMP_STREAM_API AvException final : public Exception {
public:
    explicit AvException(int32_t error);

    ~AvException() override;
};

AvException::AvException(int32_t error)
    : Exception(Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR) {
    char buf[256]{};
    LIBAV_LIB.UtilLib->av_strerror(error, buf, sizeof(buf) - 1);
    message_.assign(buf);
}

AvException::~AvException() = default;

#define AvIfFailedThrow(expr) \
	do { \
		auto error = (expr); \
		if (error != 0) { \
			throw AvException(error); \
		} \
	} while (false)

class AvFileStream::AvFileStreamImpl {
public:
    AvFileStreamImpl()
        : audio_stream_id_(-1)
        , duration_(0.0) {
        logger_ = LoggerManager::GetInstance().GetLogger(kAvFileStreamLoggerName);
    }

    ~AvFileStreamImpl() noexcept {
        Close();
    }

    void Close() noexcept {
        audio_stream_id_ = -1;
        duration_ = 0;
        audio_format_.Reset();
        swr_context_.reset();
        audio_frame_.reset();
        codec_context_.reset();
        format_context_.reset();
    }

    void LoadFromFile(std::wstring const& file_path) {
        AVFormatContext* format_ctx = nullptr;

        // todo: Http request timeout in microseconds. 
        AVDictionary* options = nullptr;
        LIBAV_LIB.UtilLib->av_dict_set(&options, "timeout", "6000000", 0);

        const auto file_path_ut8 = String::ToString(file_path);
        const auto err = LIBAV_LIB.FormatLib->avformat_open_input(&format_ctx, file_path_ut8.c_str(), nullptr, &options);
        if (err != 0) {
            if (err == AVERROR_INVALIDDATA) {
                throw NotSupportFormatException();
            }
            throw AvException(err);
        }

        if (format_ctx != nullptr) {
            format_context_.reset(format_ctx);
        }
        else {
            throw NotSupportFormatException();
        }

        if (LIBAV_LIB.FormatLib->avformat_find_stream_info(format_context_.get(), nullptr) < 0) {
            throw NotSupportFormatException();
        }

        for (uint32_t i = 0; i < format_context_->nb_streams; ++i) {
            if ((audio_stream_id_ < 0) && (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)) {
                audio_stream_id_ = i;
            }
        }

        if (HasAudio()) {
            OpenAudioStream();
        }
        else {
            throw NotSupportFormatException();
        }

        const auto stream = format_context_->streams[audio_stream_id_];
        duration_ = ::av_q2d(stream->time_base) * static_cast<double>(stream->duration);
    }

    void OpenAudioStream() {
        codec_context_.reset(format_context_->streams[audio_stream_id_]->codec);

        const auto codec = LIBAV_LIB.CodecLib->avcodec_find_decoder(codec_context_->codec_id);
        if (!codec) {
            throw NotSupportFormatException();
        }

        AvIfFailedThrow(LIBAV_LIB.CodecLib->avcodec_open2(codec_context_.get(), codec, nullptr));
        audio_frame_.reset(LIBAV_LIB.UtilLib->av_frame_alloc());

        switch (codec_context_->sample_fmt) {
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S32:
            XAMP_LOG_D(logger_, "Stream format = > INTERLEAVED");
            break;
        case AV_SAMPLE_FMT_S16P:
        case AV_SAMPLE_FMT_FLTP:
            XAMP_LOG_D(logger_, "Stream format => DEINTERLEAVED");
            break;
        default:
            throw NotSupportFormatException();
        }

        if (format_context_->iformat != nullptr) {
            XAMP_LOG_D(logger_, "Stream input format => {} bitdetph:{} bitrate:{} Kbps",
                format_context_->iformat->name,
                GetBitDepth(),
                Round(GetBitRate() / 1024.0));
        }

        auto channel_layout = codec_context_->channel_layout == 0 ? AV_CH_LAYOUT_STEREO : codec_context_->channel_layout;

        swr_context_.reset(LIBAV_LIB.SwrLib->swr_alloc_set_opts(swr_context_.get(),
            AV_CH_LAYOUT_STEREO,
            AV_SAMPLE_FMT_FLT,
            codec_context_->sample_rate,
            channel_layout,
            codec_context_->sample_fmt,
            codec_context_->sample_rate,
            0,
            nullptr));

        AvIfFailedThrow(LIBAV_LIB.SwrLib->swr_init(swr_context_.get()));
        audio_format_.SetFormat(DataFormat::FORMAT_PCM);
        audio_format_.SetChannel(static_cast<uint16_t>(codec_context_->channels));
        audio_format_.SetSampleRate(static_cast<uint32_t>(codec_context_->sample_rate));
        audio_format_.SetBitPerSample(static_cast<uint32_t>(LIBAV_LIB.UtilLib->av_get_bytes_per_sample(codec_context_->sample_fmt) * 8));
        audio_format_.SetPackedFormat(PackedFormat::INTERLEAVED);
        XAMP_LOG_D(logger_, "Stream format: {}", audio_format_);

        packet_.reset(LIBAV_LIB.CodecLib->av_packet_alloc());
        LIBAV_LIB.CodecLib->av_init_packet(packet_.get());
        is_eof_ = false;
    }

    uint32_t GetSamples(float* buffer, uint32_t length) noexcept {
        const auto channel_read_bytes = length / sizeof(float);
        uint32_t num_read_sample = 0;

        for (uint32_t i = 0; i < format_context_->nb_streams; ++i) {
            int ret = 0;
            while (ret >= 0) {
                ret = LIBAV_LIB.FormatLib->av_read_frame(format_context_.get(), packet_.get());
                if (ret == AVERROR_EOF) {
                    is_eof_ = true;
                    return 0;
                }

                XAMP_ON_SCOPE_EXIT(LIBAV_LIB.CodecLib->av_packet_unref(packet_.get()));

                if (packet_->stream_index != audio_stream_id_) {
                    continue;
                }

                auto ret = LIBAV_LIB.CodecLib->avcodec_send_packet(codec_context_.get(), packet_.get());
                if (ret == AVERROR(EAGAIN)) {
                    break;
                }

                if (ret == AVERROR_EOF || ret == AVERROR(EINVAL)) {
                    return num_read_sample;
                }

                if (ret == AVERROR_INVALIDDATA) {
                    break;
                }

                while (ret >= 0) {
                    XAMP_ON_SCOPE_EXIT(LIBAV_LIB.UtilLib->av_frame_unref(audio_frame_.get()));

                    ret = LIBAV_LIB.CodecLib->avcodec_receive_frame(codec_context_.get(), audio_frame_.get());
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
                    if (num_read_sample >= channel_read_bytes) {
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

    uint8_t GetSampleSize() const noexcept {
        return sizeof(float);
    }

    int64_t GetBitRate() const noexcept {
        return format_context_->streams[audio_stream_id_]->codecpar->bit_rate;
    }

    uint32_t GetBitDepth() const noexcept {
        return (std::max)(format_context_->streams[audio_stream_id_]->codecpar->bits_per_coded_sample, 16);
    }

    void Seek(double stream_time) {
        if (codec_context_ == nullptr) {
            return;
        }

        auto timestamp = static_cast<int64_t>(stream_time * AV_TIME_BASE);

        if (format_context_->start_time != AV_NOPTS_VALUE) {
            timestamp += format_context_->start_time;
        }

        AvIfFailedThrow(LIBAV_LIB.FormatLib->av_seek_frame(format_context_.get(), -1, timestamp, AVSEEK_FLAG_BACKWARD));
        LIBAV_LIB.CodecLib->avcodec_flush_buffers(codec_context_.get());
        is_eof_ = false;
    }

    bool IsActive() const {
        return !is_eof_;
    }
private:
    [[nodiscard]] bool HasAudio() const noexcept {
        return audio_stream_id_ >= 0;
    }

    uint32_t ConvertSamples(float* buffer, uint32_t length) const noexcept {
        if (audio_frame_->nb_samples > length) {
            return 0;
        }
        const auto frame_size = static_cast<uint32_t>(audio_frame_->nb_samples * codec_context_->channels);
        const auto result = LIBAV_LIB.SwrLib->swr_convert(swr_context_.get(),
            reinterpret_cast<uint8_t**>(&buffer),
            audio_frame_->nb_samples,
            const_cast<const uint8_t**>(audio_frame_->data),
            audio_frame_->nb_samples);
        if (result <= 0) {
            return 0;
        }
        return frame_size;
    }

    bool is_eof_{ false };
    int32_t audio_stream_id_;
    double duration_;
    AudioFormat audio_format_;
    AvPtr<SwrContext> swr_context_;
    AvPtr<AVFrame> audio_frame_;
    AvPtr<AVCodecContext> codec_context_;
    AvPtr<AVFormatContext> format_context_;
    AvPtr<AVPacket> packet_;
    std::shared_ptr<Logger> logger_;
};


AvFileStream::AvFileStream()
    : impl_(MakeAlign<AvFileStreamImpl>()) {
}

XAMP_PIMPL_IMPL(AvFileStream)

void AvFileStream::OpenFile(const std::wstring& file_path) {
    return impl_->LoadFromFile(file_path);
}

void AvFileStream::Close() noexcept {
    impl_->Close();
}

double AvFileStream::GetDuration() const {
    return impl_->GetDuration();
}

AudioFormat AvFileStream::GetFormat() const noexcept {
    return impl_->GetFormat();
}

uint32_t AvFileStream::GetSamples(void* buffer, uint32_t length) const noexcept {
    return impl_->GetSamples(static_cast<float *>(buffer), length);
}

void AvFileStream::Seek(double stream_time) const {
    impl_->Seek(stream_time);
}

std::string_view AvFileStream::GetDescription() const noexcept {
    return "AvFileStream";
}

uint8_t AvFileStream::GetSampleSize() const noexcept {
    return impl_->GetSampleSize();
}

bool AvFileStream::IsActive() const noexcept {
    return impl_->IsActive();
}

uint32_t AvFileStream::GetBitDepth() const {
    return impl_->GetBitDepth();
}

}
