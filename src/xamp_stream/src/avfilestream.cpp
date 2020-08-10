#include <stream/avfilestream.h>

#if ENABLE_FFMPEG

#ifdef XAMP_OS_WIN
#pragma comment(lib, "crypt32")
#pragma comment(lib, "Bcrypt")
#endif

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/file.h>
#include <libswresample/swresample.h>
#include <libavutil/rational.h>
#include <libavutil/opt.h>
}

#include <cassert>
#include <base/exception.h>
#include <base/memory_mapped_file.h>
#include <base/str_utilts.h>
#include <base/scopeguard.h>
#include <base/logger.h>

#include <stream/avexception.h>

namespace xamp::stream {

template <typename T>
struct AvResourceDeleter;

template <>
struct AvResourceDeleter<AVFormatContext> {
    void operator()(AVFormatContext* p) const {
        ::avformat_close_input(&p);
    }
};

template <>
struct AvResourceDeleter<AVCodecContext> {
    void operator()(AVCodecContext* p) const {
        ::avcodec_close(p);
    }
};

template <>
struct AvResourceDeleter<SwrContext> {
    void operator()(SwrContext* p) const {
        ::swr_free(&p);
    }
};

template <>
struct AvResourceDeleter<AVPacket> {
    void operator()(AVPacket* p) const {
        assert(p != nullptr);
        ::av_packet_unref(p);
        ::av_packet_free(&p);
    }
};

template <>
struct AvResourceDeleter<AVFrame> {
    void operator()(AVFrame* p) const {
        assert(p != nullptr);
        ::av_free(p);
    }
};

template <typename T>
using AvPtr = std::unique_ptr<T, AvResourceDeleter<T>>;

class LibAv final {
public:
    XAMP_DISABLE_COPY(LibAv)

    static XAMP_ALWAYS_INLINE LibAv& Instance() {
        static LibAv instance;
        return instance;
    }

protected:
    LibAv() {
#ifdef _DEBUG
        av_log_set_level(AV_LOG_VERBOSE);
        log_level = AV_LOG_VERBOSE;
#else
        ::av_log_set_level(AV_LOG_FATAL);
        log_level = AV_LOG_FATAL;
#endif
        ::av_log_set_callback(AvLogCallback);
    }

private:
    static void AvLogCallback(void*, int level, const char* fmt, va_list args) {
        if (level > log_level) {
            return;
        }
        constexpr int32_t kMsgBufSize = 1024;
        char msgbuf[kMsgBufSize]{};
        vsnprintf(msgbuf, sizeof(msgbuf) - 1, fmt, args);
        auto s = strstr(msgbuf, "\n");
        if (s != nullptr) {
            s[0] = '\0';
        }
        XAMP_LOG_DEBUG(msgbuf);
    }

    static int32_t log_level;
};

int32_t LibAv::log_level = AV_LOG_INFO;

class AvFileStream::AvFileStreamImpl {
public:
    AvFileStreamImpl()
        : audio_stream_id_(-1)
        , duration_(0.0) {
        LibAv::Instance();
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
        codec_contex_.reset();
        format_context_.reset();
    }

    void LoadFromFile(std::wstring const & file_path) {
        AVFormatContext* format_ctx = nullptr;

        auto file_path_ut8 = ToString(file_path);
        auto err = ::avformat_open_input(&format_ctx, file_path_ut8.c_str(), nullptr, nullptr);
        if (err != 0) {
            if (err == AVERROR_INVALIDDATA) {
                throw NotSupportFormatException();
            }
            throw AvException(err);
        }

        if (format_ctx != nullptr) {
            format_context_.reset(format_ctx);
        } else {
            throw NotSupportFormatException();
        }

        if (::avformat_find_stream_info(format_context_.get(), nullptr) < 0) {
            throw NotSupportFormatException();
        }

        for (uint32_t i = 0; i < format_context_->nb_streams; ++i) {
            if ((audio_stream_id_ < 0) && (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)) {
                audio_stream_id_ = i;
            }
        }

        if (HasAudio()) {
            OpenAudioStream();
        } else {
            throw NotSupportFormatException();
        }

        const auto stream = format_context_->streams[audio_stream_id_];
        duration_ = ::av_q2d(stream->time_base) * static_cast<double>(stream->duration);
    }

    void OpenAudioStream() {
        codec_contex_.reset(format_context_->streams[audio_stream_id_]->codec);

        const auto codec = ::avcodec_find_decoder(codec_contex_->codec_id);
        if (!codec) {
            throw NotSupportFormatException();
        }

        AvIfFailedThrow(::avcodec_open2(codec_contex_.get(), codec, nullptr));
        audio_frame_.reset(av_frame_alloc());

        switch (codec_contex_->sample_fmt) {
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S32:
            XAMP_LOG_DEBUG("Stream format => INTERLEAVED");
            break;
        case AV_SAMPLE_FMT_S16P:
        case AV_SAMPLE_FMT_FLTP:
            XAMP_LOG_DEBUG("Stream format => DEINTERLEAVED");
            break;
        default:
            throw NotSupportFormatException();
        }

        if (format_context_->iformat != nullptr) {
            XAMP_LOG_DEBUG("Stream input format => {}", format_context_->iformat->name);
        }

        auto channel_layout = codec_contex_->channel_layout == 0 ? AV_CH_LAYOUT_STEREO : codec_contex_->channel_layout;
        // 固定轉成INTERLEAVED格式
        swr_context_.reset(::swr_alloc_set_opts(swr_context_.get(),
                                                AV_CH_LAYOUT_STEREO,
                                                AV_SAMPLE_FMT_FLT,
                                                codec_contex_->sample_rate,
                                                channel_layout,
                                                codec_contex_->sample_fmt,
                                                codec_contex_->sample_rate,
                                                0,
                                                nullptr));

        AvIfFailedThrow(::swr_init(swr_context_.get()));
        audio_format_.SetFormat(DataFormat::FORMAT_PCM);
        audio_format_.SetChannel(static_cast<uint32_t>(codec_contex_->channels));
        audio_format_.SetSampleRate(static_cast<uint32_t>(codec_contex_->sample_rate));
        audio_format_.SetBitPerSample(static_cast<uint32_t>(::av_get_bytes_per_sample(codec_contex_->sample_fmt) * 8));
        audio_format_.SetInterleavedFormat(InterleavedFormat::INTERLEAVED);
        XAMP_LOG_DEBUG("Stream format: {}", audio_format_);        
    }

    uint32_t GetSamples(float* buffer, uint32_t length) noexcept {
        uint32_t num_read_sample = 0;

        const auto need_length = length / 4;

        for (uint32_t i = 0; i < format_context_->nb_streams; ++i) {
            AvPtr<AVPacket> packet(::av_packet_alloc());
            ::av_init_packet(packet.get());

            while (::av_read_frame(format_context_.get(), packet.get()) >= 0) {
                XAMP_ON_SCOPE_EXIT(::av_packet_unref(packet.get()));

                if (packet->stream_index != audio_stream_id_) {
                    continue;
                }

                auto ret = ::avcodec_send_packet(codec_contex_.get(), packet.get());
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
                    XAMP_ON_SCOPE_EXIT(::av_frame_unref(audio_frame_.get()));

                    ret = ::avcodec_receive_frame(codec_contex_.get(), audio_frame_.get());
                    if (ret == AVERROR(EAGAIN)) {
                        break;
                    }

                    if (ret == AVERROR_EOF || ret == AVERROR(EINVAL)) {
                        return num_read_sample;
                    }

                    auto convert_size = ConvertSamples(buffer + num_read_sample, length - num_read_sample);
                    if (convert_size == 0) {
                        return 0;
                    }
                    num_read_sample += convert_size;
                    if (num_read_sample >= need_length) {
                        return num_read_sample;
                    }
                }
            }
        }
        return num_read_sample;
    }

    [[nodiscard]] AudioFormat GetFormat() const noexcept {
        assert(codec_contex_ != nullptr);
        return audio_format_;
    }

    [[nodiscard]] double GetDuration() const noexcept {
        assert(codec_contex_ != nullptr);
        return duration_;
    }

    [[nodiscard]] uint32_t GetSampleSize() const noexcept {
        return sizeof(float);
    }

    void Seek(double stream_time) const {
        if (codec_contex_ == nullptr) {
            return;
        }

        auto timestamp = static_cast<int64_t>(stream_time * AV_TIME_BASE);

        if (format_context_->start_time != AV_NOPTS_VALUE) {
            timestamp += format_context_->start_time;
        }

        AvIfFailedThrow(::av_seek_frame(format_context_.get(), -1, timestamp, AVSEEK_FLAG_BACKWARD));
        ::avcodec_flush_buffers(codec_contex_.get());
    }

private:
    [[nodiscard]] bool HasAudio() const noexcept {
        return audio_stream_id_ >= 0;
    }

    uint32_t ConvertSamples(float* buffer, uint32_t length) const noexcept {
        const uint32_t frame_size = static_cast<uint32_t>(audio_frame_->nb_samples * codec_contex_->channels);
        const auto result = ::swr_convert(swr_context_.get(),
                                          reinterpret_cast<uint8_t **>(&buffer),
                                          audio_frame_->nb_samples,
                                          const_cast<const uint8_t **>(audio_frame_->extended_data),
                                          audio_frame_->nb_samples);
        if (result <= 0) {
            return 0;
        }
        assert(result > 0);
        const uint32_t convert_size = static_cast<uint32_t>(result * codec_contex_->channels);
        assert(convert_size == frame_size && convert_size <= length);
        return frame_size;
    }

    int32_t audio_stream_id_;
    double duration_;
    AudioFormat audio_format_;
    AvPtr<SwrContext> swr_context_;
    AvPtr<AVFrame> audio_frame_;
    AvPtr<AVCodecContext> codec_contex_;
    AvPtr<AVFormatContext> format_context_;
};

AvFileStream::AvFileStream()
    : impl_(MakeAlign<AvFileStreamImpl>()) {
}

XAMP_PIMPL_IMPL(AvFileStream)

void AvFileStream::OpenFromFile(const std::wstring& file_path) {
    return impl_->LoadFromFile(file_path);
}

void AvFileStream::Close() {
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
    return "LibAv";
}

uint32_t AvFileStream::GetSampleSize() const noexcept {
    return impl_->GetSampleSize();
}

}

#endif

