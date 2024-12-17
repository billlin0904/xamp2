#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/buffer.h>
#include <base/dataconverter.h>
#include <base/port.h>
#include <stream/avlib.h>
#include <stream/api.h>
#include <stream/filestream.h>
#include <stream/alacencoder.h>
#include <stream/bassfilestream.h>

XAMP_STREAM_NAMESPACE_BEGIN

namespace {
    // ALAC extradata size is 36 bytes
    const int ALAC_EXTRADATA_SIZE = 36;

    inline void bytestream_put_byte(uint8_t** buf, uint8_t value) {
        **buf = value;
        (*buf)++;
    }

    inline void bytestream_put_be16(uint8_t** buf, uint16_t value) {
        (*buf)[0] = static_cast<uint8_t>(value >> 8);
        (*buf)[1] = static_cast<uint8_t>(value & 0xFF);
        *buf += 2;
    }

    inline void bytestream_put_be32(uint8_t** buf, uint32_t value) {
        (*buf)[0] = static_cast<uint8_t>(value >> 24);
        (*buf)[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
        (*buf)[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
        (*buf)[3] = static_cast<uint8_t>(value & 0xFF);
        *buf += 4;
    }
}

class AlacFileEncoder::AlacFileEncoderImpl {
public:
    ~AlacFileEncoderImpl() {
        codec_context_.reset();
        output_io_context_.reset();
        format_context_.reset();
    }

    void Start(const AnyMap& config) {
        const auto input_file_path = config.AsPath(FileEncoderConfig::kInputFilePath);
        const auto output_file_path = config.AsPath(FileEncoderConfig::kOutputFilePath);
        file_name_ = String::ToUtf8String(output_file_path.wstring());
        input_file_ = StreamFactory::MakeFileStream(input_file_path, DsdModes::DSD_MODE_PCM);
        input_file_->OpenFile(input_file_path);
        CreateAudioStream();
    }

    void CreateAudioStream() {
        const auto frame_size = 4096;
        const auto channel_layout = AV_CH_LAYOUT_STEREO; // stereo
        const char guess_file_name[] = "output.m4a";
        const char* file_name = file_name_.c_str();

        auto best_sample_format = AV_SAMPLE_FMT_S16P;
        auto sample_size = 16;

        const auto format = input_file_->GetFormat();

        if (input_file_->GetBitDepth() > 16) {
            best_sample_format = AV_SAMPLE_FMT_S32P;
            sample_size = 24;
        }

        sample_size_ = sample_size;

        AVIOContext* output_io_context = nullptr;
        AvIfFailedThrow(LIBAV_LIB.Format->avio_open(&output_io_context, file_name, AVIO_FLAG_WRITE));
        output_io_context_.reset(output_io_context);

        format_context_.reset(LIBAV_LIB.Format->avformat_alloc_context());
        if (!format_context_) {
            throw Exception();
        }

        format_context_->pb = output_io_context;

        auto* av_codec = LIBAV_LIB.Codec->avcodec_find_encoder(AV_CODEC_ID_ALAC);
        if (!av_codec) {
            throw Exception("ALAC encoder not found.");
        }

        stream_ = LIBAV_LIB.Format->avformat_new_stream(format_context_.get(), av_codec);
        if (!stream_) {
            throw Exception("Failed to create new stream.");
        }

        codec_context_.reset(LIBAV_LIB.Codec->avcodec_alloc_context3(av_codec));
        codec_context_->channels = format.GetChannels();
        codec_context_->channel_layout = channel_layout;
        codec_context_->sample_rate = format.GetSampleRate();
        codec_context_->sample_fmt = best_sample_format;
        codec_context_->frame_size = frame_size;

        stream_->id = format_context_->nb_streams - 1;
        stream_->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
        stream_->codecpar->codec_id = AV_CODEC_ID_ALAC;
        stream_->codecpar->channel_layout = channel_layout;
        stream_->codecpar->channels = format.GetChannels();
        stream_->codecpar->sample_rate = format.GetSampleRate();
        stream_->codecpar->frame_size = frame_size;
        stream_->codecpar->format = best_sample_format;
        stream_->time_base = AVRational{ 1, static_cast<int32_t>(format.GetSampleRate()) };
        codec_context_->time_base = stream_->time_base;

        format_context_->oformat = LIBAV_LIB.Format->av_guess_format(nullptr, guess_file_name, nullptr);
        if (format_context_->oformat->flags & AVFMT_GLOBALHEADER)
            codec_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        // 設定 ALAC extradata (alacSpecificConfig)
        codec_context_->extradata = static_cast<uint8_t*>(LIBAV_LIB.Util->av_malloc(ALAC_EXTRADATA_SIZE + AV_INPUT_BUFFER_PADDING_SIZE));
        codec_context_->extradata_size = ALAC_EXTRADATA_SIZE;
        MemorySet(codec_context_->extradata, 0, ALAC_EXTRADATA_SIZE);

        uint8_t* buf = codec_context_->extradata;
        auto frame_length = frame_size; // ensure consistent with frame_size
        auto avg_bitrate = format.GetSampleRate() * format.GetChannels() * sample_size; // bits per second
        auto sr = format.GetSampleRate();

        // Write alacSpecificConfig in big-endian
        bytestream_put_be32(&buf, frame_length); // 4 bytes
        bytestream_put_byte(&buf, 0);            // compatibleVersion
        bytestream_put_byte(&buf, sample_size);  // bitDepth (16)
        bytestream_put_byte(&buf, 40);           // pb
        bytestream_put_byte(&buf, 10);           // mb
        bytestream_put_byte(&buf, 14);           // kb (rice_param_limit)
        bytestream_put_byte(&buf, format.GetChannels()); // channels
        bytestream_put_be16(&buf, 255);          // maxRun
        bytestream_put_be32(&buf, 0);            // maxFrameBytes (0=unknown)
        bytestream_put_be32(&buf, avg_bitrate);  // avgBitrate (bits/sec)
        bytestream_put_be32(&buf, sr);           // sampleRate (big-endian)

        // Padding to reach 36 bytes total
        for (int i = 0; i < 12; i++) {
            bytestream_put_byte(&buf, 0);
        }      

        // 先開啟編碼器
        AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_open2(codec_context_.get(), av_codec, nullptr));

        // 接著從 codec_context_ 將參數(包含 extradata) 拷貝到 codecpar
        AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_parameters_from_context(stream_->codecpar, codec_context_.get()));

        // 寫入文件標頭
        AvIfFailedThrow(LIBAV_LIB.Format->avformat_write_header(format_context_.get(), nullptr));
    }

    bool EncodeFrame(AVFrame* frame) {
        AvPtr<AVPacket> packet;
        packet.reset(LIBAV_LIB.Codec->av_packet_alloc());
        LIBAV_LIB.Codec->av_init_packet(packet.get());

        if (frame != nullptr) {
            frame->pts = LIBAV_LIB.Util->av_rescale_q(pts_, AVRational{ 1, frame->sample_rate }, codec_context_->time_base);
            pts_ += frame->nb_samples;
        }

        auto ret = LIBAV_LIB.Codec->avcodec_send_frame(codec_context_.get(), frame);
        AvIfFailedThrow(ret);

        ret = LIBAV_LIB.Codec->avcodec_receive_packet(codec_context_.get(), packet.get());
        if (ret == AVERROR_EOF) {
            return false;
        }
        if (ret == AVERROR(EAGAIN)) {
            return true;
        }
        else {
            AvIfFailedThrow(ret);
        }

        ret = LIBAV_LIB.Format->av_interleaved_write_frame(format_context_.get(), packet.get());
        AvIfFailedThrow(ret);
        return true;
    }

    void Encode(const std::function<bool(uint32_t)>& progress) {
        AvPtr<AVFrame> frame;
        frame.reset(LIBAV_LIB.Util->av_frame_alloc());
        frame->nb_samples = codec_context_->frame_size;
        frame->format = codec_context_->sample_fmt;
        frame->channel_layout = codec_context_->channel_layout;
        frame->sample_rate = codec_context_->sample_rate;

        buffer_.resize(codec_context_->frame_size * 2);

        auto format = input_file_->GetFormat();
        uint64_t total_samples = static_cast<uint64_t>(input_file_->GetDurationAsSeconds() * format.GetSampleRate() * format.GetChannels());
        uint64_t processed_samples = 0;

        auto ret = LIBAV_LIB.Util->av_frame_get_buffer(frame.get(), 0);
        if (ret < 0) {
            AvIfFailedThrow(ret);
        }

        while (true) {
            ret = LIBAV_LIB.Util->av_frame_make_writable(frame.get());
            if (ret < 0) {
                AvIfFailedThrow(ret);
            }

            const auto read_samples = input_file_->GetSamples(buffer_.data(),
                codec_context_->frame_size * 2);
            if (!read_samples) {
                break;
            }

            if (sample_size_ == 16) {
                auto* left_ptr = reinterpret_cast<int16_t*>(frame->data[0]);
                auto* right_ptr = reinterpret_cast<int16_t*>(frame->data[1]);

                for (auto i = 0; i < read_samples; i += 2) {
                    *left_ptr++ = static_cast<int16_t>(buffer_[i + 0] * kFloat16Scale);
                    *right_ptr++ = static_cast<int16_t>(buffer_[i + 1] * kFloat16Scale);
                }
            } else {
                auto* left_ptr = reinterpret_cast<int32_t*>(frame->data[0]);
                auto* right_ptr = reinterpret_cast<int32_t*>(frame->data[1]);

                for (auto i = 0; i < read_samples; i += 2) {
                    *left_ptr++ = static_cast<int32_t>(buffer_[i + 0] * kFloat24Scale) << 8;
                    *right_ptr++ = static_cast<int32_t>(buffer_[i + 1] * kFloat24Scale) << 8;
                }
            }

            if (!EncodeFrame(frame.get())) {
                break;
            }

            processed_samples += read_samples;

            uint32_t percentage = 0;
            if (total_samples > 0) {
                percentage = static_cast<uint32_t>((static_cast<double>(processed_samples) / static_cast<double>(total_samples)) * 100);
            }

            if (!progress(percentage)) {
                break;
            }
        }

        // 送入NULL frame給encoder讓其flush
        while (EncodeFrame(nullptr)) {
            // 不斷接收flush出來的packet，直到返回false
        }

        AvIfFailedThrow(LIBAV_LIB.Format->av_write_trailer(format_context_.get()));
    }

private:
    uint32_t sample_size_{ 0 };
    int64_t pts_{ 0 };
    AVStream* stream_{ nullptr };
    std::string file_name_;
    AvPtr<AVCodecContext> codec_context_;
    AvPtr<AVIOContext> output_io_context_;
    AvPtr<AVFormatContext> format_context_;
    ScopedPtr<FileStream> input_file_;
    Buffer<float> buffer_;
};

AlacFileEncoder::AlacFileEncoder()
    : impl_(MakeAlign<AlacFileEncoderImpl>()) {
}

XAMP_PIMPL_IMPL(AlacFileEncoder)

void AlacFileEncoder::Start(const AnyMap& config) {
    impl_->Start(config);
}

void AlacFileEncoder::Encode(std::function<bool(uint32_t)> const& progress) {
    impl_->Encode(progress);
}

XAMP_STREAM_NAMESPACE_END
