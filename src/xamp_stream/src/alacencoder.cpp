#include <immintrin.h>

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
    constexpr int kAlacExtradataSize = 36;

    void bytestream_put_byte(uint8_t** buf, uint8_t value) {
        **buf = value;
        (*buf)++;
    }

    void bytestream_put_be16(uint8_t** buf, uint16_t value) {
        (*buf)[0] = static_cast<uint8_t>(value >> 8);
        (*buf)[1] = static_cast<uint8_t>(value & 0xFF);
        *buf += 2;
    }

    void bytestream_put_be32(uint8_t** buf, uint32_t value) {
        (*buf)[0] = static_cast<uint8_t>(value >> 24);
        (*buf)[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
        (*buf)[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
        (*buf)[3] = static_cast<uint8_t>(value & 0xFF);
        *buf += 4;
    }

    void ConvertFloatToInt16SSE(const float* input, int16_t* left_ptr, int16_t* right_ptr, size_t frames) {
        // �C���B�z4 frames = 8 floats (L,R interleaved)
        // �������X���B�k�n�D
        // ��ƱƦC: L0,R0,L1,R1,L2,R2,L3,R3
        // �ڭ̥i�H�z�L shuffle ���O�N����index��X (left)�A�_��index��X (right)�C

        // shuffle mask:
        // �ϥ� _mm_shuffle_ps �i�b128-bit (4 floats) �����աA���ڭ̦�8 floats�b��� __m128��
        // �����G�����J��� __m128:
        // in1 = L0,R0,L1,R1
        // in2 = L2,R2,L3,R3
        // ���N in1�Bin2 �U�۩��left�Bright�n�D�A�A�X�֡C

        size_t i = 0;
        for (; i + 4 <= frames; i += 4) {
            __m128 in1 = _mm_loadu_ps(input);        // L0,R0,L1,R1
            __m128 in2 = _mm_loadu_ps(input + 4);    // L2,R2,L3,R3

            // �Y��
            __m128 scale = _mm_set1_ps(kFloat16Scale);
            in1 = _mm_mul_ps(in1, scale);
            in2 = _mm_mul_ps(in2, scale);

            // �N in1�Bin2 �������k�n�D
            // Left channel: �� in1 �� (L0,L1) �P in2 �� (L2,L3)
            // Right channel: �� in1 �� (R0,R1) �P in2 �� (R2,R3)

            // in1: L0,R0,L1,R1   �Q�����n�D(L0,L1): ����index 0,2
            // _mm_shuffle_ps(in1, in1, _MM_SHUFFLE(2,0,2,0)) �i���o (L0,L1,L0,L1)
            __m128 left_part1 = _mm_shuffle_ps(in1, in1, _MM_SHUFFLE(2, 0, 2, 0));
            // �P�z right_part1 = (R0,R1,R0,R1)
            __m128 right_part1 = _mm_shuffle_ps(in1, in1, _MM_SHUFFLE(3, 1, 3, 1));

            // in2: L2,R2,L3,R3
            __m128 left_part2 = _mm_shuffle_ps(in2, in2, _MM_SHUFFLE(2, 0, 2, 0));   // (L2,L3,L2,L3)
            __m128 right_part2 = _mm_shuffle_ps(in2, in2, _MM_SHUFFLE(3, 1, 3, 1));   // (R2,R3,R2,R3)

            // �{�b�n�N (L0,L1) �P (L2,L3) �X�֦� (L0,L1,L2,L3)
            // left_part1: (L0,L1,L0,L1)
            // left_part2: (L2,L3,L2,L3)
            // �i�� _mm_unpacklo_ps�B_mm_movehl_ps ����k�N�e��Ӥ������X

            // ��k�G�����ݭn���e���float
            // L0,L1 �b left_part1���e���float
            // L2,L3 �b left_part2���e���float

            // _mm_unpacklo_ps ����� __m128 ���C���float��´
            __m128 left_final = _mm_unpacklo_ps(left_part1, left_part2);
            // left_final = (L0,L2,L1,L3)
            // �A�ϥ� shuffle �N���ǽզ� (L0,L1,L2,L3)
            left_final = _mm_shuffle_ps(left_final, left_final, _MM_SHUFFLE(2, 0, 3, 1));
            // �{�b left_final = (L0,L1,L2,L3)

            // �� right channel ���P�˳B�z
            __m128 right_final = _mm_unpacklo_ps(right_part1, right_part2);
            // right_final = (R0,R2,R1,R3)
            right_final = _mm_shuffle_ps(right_final, right_final, _MM_SHUFFLE(2, 0, 3, 1));
            // right_final = (R0,R1,R2,R3)

            // �{�b left_final, right_final �O float(L0,L1,L2,L3) and float(R0,R1,R2,R3)
            // �N float -> int32 -> int16
            __m128i left_i32 = _mm_cvtps_epi32(left_final);
            __m128i right_i32 = _mm_cvtps_epi32(right_final);

            // �Nint32���]��int16
            __m128i packed = _mm_packs_epi32(left_i32, right_i32);
            // packed: �e4��int16����left channel, ��4��int16����right channel?

            // packs_epi32����:
            // left_i32��4��int32����4��int16�bpacked�C64-bit
            // right_i32��4��int32����4��int16�bpacked��64-bit
            // �{�b�e4��int16�O���n�D(L0,L1,L2,L3)�A��4��int16�O�k�n�D(R0,R1,R2,R3)

            // �N�e4��int16�s�J left_ptr
            _mm_storel_epi64(reinterpret_cast<__m128i*>(left_ptr), packed);
            left_ptr += 4;
            // �N��4��int16����C64bit��m��s�J right_ptr
            __m128i right_shifted = _mm_unpackhi_epi64(packed, packed);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(right_ptr), right_shifted);
            right_ptr += 4;

            input += 8;  // �e�i8��float(4 frames*2ch)
        }

        // ���ݤ���4 frames�жq�B�z
        for (; i < frames; i++) {
            float L = input[0] * kFloat16Scale;
            float R = input[1] * kFloat16Scale;
            *left_ptr++ = static_cast<int16_t>(L);
            *right_ptr++ = static_cast<int16_t>(R);
            input += 2;
        }
    }

    void ConvertFloatToInt24SSE(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames) {
        // �C���B�z4 frames = 8 floats: L0,R0,L1,R1,L2,R2,L3,R3
        // �P16bit���B�z�����A������pack�Y��int16�A�u����int32�� << 8

        size_t i = 0;
        __m128 scale = _mm_set1_ps(kFloat24Scale);

        for (; i + 4 <= frames; i += 4) {
            __m128 in1 = _mm_loadu_ps(input);       // L0,R0,L1,R1
            __m128 in2 = _mm_loadu_ps(input + 4);   // L2,R2,L3,R3

            in1 = _mm_mul_ps(in1, scale);
            in2 = _mm_mul_ps(in2, scale);

            // �������n�D(L0,L1,L2,L3)�M�k�n�D(R0,R1,R2,R3)
            // �M16bit�����P�˪�������k�A�i�����M��
            __m128 left_part1 = _mm_shuffle_ps(in1, in1, _MM_SHUFFLE(2, 0, 2, 0)); // (L0,L1,L0,L1)
            __m128 left_part2 = _mm_shuffle_ps(in2, in2, _MM_SHUFFLE(2, 0, 2, 0)); // (L2,L3,L2,L3)
            __m128 right_part1 = _mm_shuffle_ps(in1, in1, _MM_SHUFFLE(3, 1, 3, 1)); // (R0,R1,R0,R1)
            __m128 right_part2 = _mm_shuffle_ps(in2, in2, _MM_SHUFFLE(3, 1, 3, 1)); // (R2,R3,R2,R3)

            __m128 left_merge = _mm_unpacklo_ps(left_part1, left_part2); // (L0,L2,L1,L3)
            left_merge = _mm_shuffle_ps(left_merge, left_merge, _MM_SHUFFLE(2, 0, 3, 1)); // (L0,L1,L2,L3)

            __m128 right_merge = _mm_unpacklo_ps(right_part1, right_part2); // (R0,R2,R1,R3)
            right_merge = _mm_shuffle_ps(right_merge, right_merge, _MM_SHUFFLE(2, 0, 3, 1)); // (R0,R1,R2,R3)

            __m128i left_i32 = _mm_cvtps_epi32(left_merge);
            __m128i right_i32 = _mm_cvtps_epi32(right_merge);

            // �N24bit���: << 8
            __m128i shift8 = _mm_slli_epi32(left_i32, 8);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(left_ptr), shift8);
            left_ptr += 4;

            shift8 = _mm_slli_epi32(right_i32, 8);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(right_ptr), shift8);
            right_ptr += 4;

            input += 8;
        }

        // ���ݤ���4 frames�жq�B�z
        for (; i < frames; i++) {
            float L = input[0] * kFloat24Scale;
            float R = input[1] * kFloat24Scale;
            int32_t Li = static_cast<int32_t>(L) << 8;
            int32_t Ri = static_cast<int32_t>(R) << 8;
            *left_ptr++ = Li;
            *right_ptr++ = Ri;
            input += 2;
        }
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
            convert_ = [](const float* input, void* left_ch, void* right_ch, size_t read_samples) {
                ConvertFloatToInt24SSE(input,
                    static_cast<int32_t*>(left_ch),
                    static_cast<int32_t*>(right_ch),
                    read_samples);
                };
        } else {
            convert_ = [](const float* input, void* left_ch, void* right_ch, size_t read_samples) {
                ConvertFloatToInt16SSE(input,
                    static_cast<int16_t*>(left_ch),
                    static_cast<int16_t*>(right_ch),
                    read_samples);
                };
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

        // �]�w ALAC extradata (alacSpecificConfig)
        codec_context_->extradata = static_cast<uint8_t*>(LIBAV_LIB.Util->av_malloc(kAlacExtradataSize + AV_INPUT_BUFFER_PADDING_SIZE));
        codec_context_->extradata_size = kAlacExtradataSize;
        MemorySet(codec_context_->extradata, 0, kAlacExtradataSize);

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

        // ���}�ҽs�X��
        AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_open2(codec_context_.get(), av_codec, nullptr));
        // ���۱q codec_context_ �N�Ѽ�(�]�t extradata) ������ codecpar
        AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_parameters_from_context(stream_->codecpar, codec_context_.get()));
        // �g�J�����Y
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

            std::invoke(convert_, buffer_.data(),
                frame->data[0],
                frame->data[1],
                read_samples / 2);

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

        while (EncodeFrame(nullptr)) {
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
    std::function<void(const float*, void*, void*, size_t)> convert_;
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
