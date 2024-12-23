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
#include <stream/m4aencoder.h>
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

    void ConvertInterleavedToPlanarSSE(const float* input, float* left_ptr, float* right_ptr, size_t frames) {
        size_t i = 0;
        for (; i + 4 <= frames; i += 4) {
            __m128 in1 = _mm_loadu_ps(input);     // L0,R0,L1,R1
            __m128 in2 = _mm_loadu_ps(input + 4); // L2,R2,L3,R3

            // 分離左聲道: 從 in1, in2 抽取偶數index樣本(L0,L1,L2,L3)
            __m128 left_part1 = _mm_shuffle_ps(in1, in1, _MM_SHUFFLE(2, 0, 2, 0));  // (L0,L1,L0,L1)
            __m128 left_part2 = _mm_shuffle_ps(in2, in2, _MM_SHUFFLE(2, 0, 2, 0));  // (L2,L3,L2,L3)
            // 合併得到 (L0,L1,L2,L3)
            __m128 left_final = _mm_unpacklo_ps(left_part1, left_part2); // (L0,L2,L1,L3)
            left_final = _mm_shuffle_ps(left_final, left_final, _MM_SHUFFLE(2, 0, 3, 1)); // (L0,L1,L2,L3)

            // 分離右聲道: 從 in1, in2 抽取奇數index樣本(R0,R1,R2,R3)
            __m128 right_part1 = _mm_shuffle_ps(in1, in1, _MM_SHUFFLE(3, 1, 3, 1)); // (R0,R1,R0,R1)
            __m128 right_part2 = _mm_shuffle_ps(in2, in2, _MM_SHUFFLE(3, 1, 3, 1)); // (R2,R3,R2,R3)
            __m128 right_final = _mm_unpacklo_ps(right_part1, right_part2); // (R0,R2,R1,R3)
            right_final = _mm_shuffle_ps(right_final, right_final, _MM_SHUFFLE(2, 0, 3, 1)); // (R0,R1,R2,R3)

            // 將結果存回記憶體
            _mm_storeu_ps(left_ptr, left_final);
            _mm_storeu_ps(right_ptr, right_final);

            left_ptr += 4;
            right_ptr += 4;
            input += 8; // 前進8個float (4 frames * 2)
        }

        // 處理不足4 frames的尾巴
        for (; i < frames; i++) {
            left_ptr[0] = input[0];
            right_ptr[0] = input[1];
            left_ptr++;
            right_ptr++;
            input += 2;
        }
    }

    void ConvertFloatToInt16SSE(const float* input, int16_t* left_ptr, int16_t* right_ptr, size_t frames) {
        // 每次處理4 frames = 8 floats (L,R interleaved)
        // 須分離出左、右聲道
        // 資料排列: L0,R0,L1,R1,L2,R2,L3,R3
        // 我們可以透過 shuffle 指令將偶數index抽出 (left)，奇數index抽出 (right)。

        // shuffle mask:
        // 使用 _mm_shuffle_ps 可在128-bit (4 floats) 內重組，但我們有8 floats在兩個 __m128中
        // 策略：先載入兩組 __m128:
        // in1 = L0,R0,L1,R1
        // in2 = L2,R2,L3,R3
        // 先將 in1、in2 各自抽取left、right聲道，再合併。

        size_t i = 0;
        for (; i + 4 <= frames; i += 4) {
            __m128 in1 = _mm_loadu_ps(input);        // L0,R0,L1,R1
            __m128 in2 = _mm_loadu_ps(input + 4);    // L2,R2,L3,R3

            // 縮放
            __m128 scale = _mm_set1_ps(kFloat16Scale);
            in1 = _mm_mul_ps(in1, scale);
            in2 = _mm_mul_ps(in2, scale);

            // 將 in1、in2 分離左右聲道
            // Left channel: 取 in1 的 (L0,L1) 與 in2 的 (L2,L3)
            // Right channel: 取 in1 的 (R0,R1) 與 in2 的 (R2,R3)

            // in1: L0,R0,L1,R1   想取左聲道(L0,L1): 偶數index 0,2
            // _mm_shuffle_ps(in1, in1, _MM_SHUFFLE(2,0,2,0)) 可取得 (L0,L1,L0,L1)
            __m128 left_part1 = _mm_shuffle_ps(in1, in1, _MM_SHUFFLE(2, 0, 2, 0));
            // 同理 right_part1 = (R0,R1,R0,R1)
            __m128 right_part1 = _mm_shuffle_ps(in1, in1, _MM_SHUFFLE(3, 1, 3, 1));

            // in2: L2,R2,L3,R3
            __m128 left_part2 = _mm_shuffle_ps(in2, in2, _MM_SHUFFLE(2, 0, 2, 0));   // (L2,L3,L2,L3)
            __m128 right_part2 = _mm_shuffle_ps(in2, in2, _MM_SHUFFLE(3, 1, 3, 1));   // (R2,R3,R2,R3)

            // 現在要將 (L0,L1) 與 (L2,L3) 合併成 (L0,L1,L2,L3)
            // left_part1: (L0,L1,L0,L1)
            // left_part2: (L2,L3,L2,L3)
            // 可用 _mm_unpacklo_ps、_mm_movehl_ps 等方法將前兩個元素取出

            // 方法：先取需要的前兩個float
            // L0,L1 在 left_part1的前兩個float
            // L2,L3 在 left_part2的前兩個float

            // _mm_unpacklo_ps 取兩個 __m128 的低兩個float交織
            __m128 left_final = _mm_unpacklo_ps(left_part1, left_part2);
            // left_final = (L0,L2,L1,L3)
            // 再使用 shuffle 將順序調成 (L0,L1,L2,L3)
            left_final = _mm_shuffle_ps(left_final, left_final, _MM_SHUFFLE(2, 0, 3, 1));
            // 現在 left_final = (L0,L1,L2,L3)

            // 對 right channel 做同樣處理
            __m128 right_final = _mm_unpacklo_ps(right_part1, right_part2);
            // right_final = (R0,R2,R1,R3)
            right_final = _mm_shuffle_ps(right_final, right_final, _MM_SHUFFLE(2, 0, 3, 1));
            // right_final = (R0,R1,R2,R3)

            // 現在 left_final, right_final 是 float(L0,L1,L2,L3) and float(R0,R1,R2,R3)
            // 將 float -> int32 -> int16
            __m128i left_i32 = _mm_cvttps_epi32(left_final);
            __m128i right_i32 = _mm_cvttps_epi32(right_final);

            // 將int32打包成int16
            __m128i packed = _mm_packs_epi32(left_i32, right_i32);
            // packed: 前4個int16對應left channel, 後4個int16對應right channel?

            // packs_epi32順序:
            // left_i32有4個int32壓成4個int16在packed低64-bit
            // right_i32有4個int32壓成4個int16在packed高64-bit
            // 現在前4個int16是左聲道(L0,L1,L2,L3)，後4個int16是右聲道(R0,R1,R2,R3)

            // 將前4個int16存入 left_ptr
            _mm_storel_epi64(reinterpret_cast<__m128i*>(left_ptr), packed);
            left_ptr += 4;
            // 將後4個int16移到低64bit位置後存入 right_ptr
            __m128i right_shifted = _mm_unpackhi_epi64(packed, packed);
            _mm_storel_epi64(reinterpret_cast<__m128i*>(right_ptr), right_shifted);
            right_ptr += 4;

            input += 8;  // 前進8個float(4 frames*2ch)
        }

        // 尾端不足4 frames標量處理
        for (; i < frames; i++) {
            float L = input[0] * kFloat16Scale;
            float R = input[1] * kFloat16Scale;
            *left_ptr++ = static_cast<int16_t>(L);
            *right_ptr++ = static_cast<int16_t>(R);
            input += 2;
        }
    }

    void ConvertFloatToInt24SSE(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames) {
        // 每次處理4 frames = 8 floats: L0,R0,L1,R1,L2,R2,L3,R3
        // 與16bit的處理類似，但不用pack縮成int16，只需轉int32後 << 8

        size_t i = 0;
        __m128 scale = _mm_set1_ps(kFloat24Scale);

        for (; i + 4 <= frames; i += 4) {
            __m128 in1 = _mm_loadu_ps(input);       // L0,R0,L1,R1
            __m128 in2 = _mm_loadu_ps(input + 4);   // L2,R2,L3,R3

            in1 = _mm_mul_ps(in1, scale);
            in2 = _mm_mul_ps(in2, scale);

            // 分離左聲道(L0,L1,L2,L3)和右聲道(R0,R1,R2,R3)
            // 和16bit版本同樣的分離方法，可直接套用
            __m128 left_part1 = _mm_shuffle_ps(in1, in1, _MM_SHUFFLE(2, 0, 2, 0)); // (L0,L1,L0,L1)
            __m128 left_part2 = _mm_shuffle_ps(in2, in2, _MM_SHUFFLE(2, 0, 2, 0)); // (L2,L3,L2,L3)
            __m128 right_part1 = _mm_shuffle_ps(in1, in1, _MM_SHUFFLE(3, 1, 3, 1)); // (R0,R1,R0,R1)
            __m128 right_part2 = _mm_shuffle_ps(in2, in2, _MM_SHUFFLE(3, 1, 3, 1)); // (R2,R3,R2,R3)

            __m128 left_merge = _mm_unpacklo_ps(left_part1, left_part2); // (L0,L2,L1,L3)
            left_merge = _mm_shuffle_ps(left_merge, left_merge, _MM_SHUFFLE(2, 0, 3, 1)); // (L0,L1,L2,L3)

            __m128 right_merge = _mm_unpacklo_ps(right_part1, right_part2); // (R0,R2,R1,R3)
            right_merge = _mm_shuffle_ps(right_merge, right_merge, _MM_SHUFFLE(2, 0, 3, 1)); // (R0,R1,R2,R3)

            __m128i left_i32 = _mm_cvttps_epi32(left_merge);
            __m128i right_i32 = _mm_cvttps_epi32(right_merge);

            // 將24bit對齊: << 8
            __m128i shift8 = _mm_slli_epi32(left_i32, 8);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(left_ptr), shift8);
            left_ptr += 4;

            shift8 = _mm_slli_epi32(right_i32, 8);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(right_ptr), shift8);
            right_ptr += 4;

            input += 8;
        }

        // 尾端不足4 frames標量處理
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

    int32_t CustomReadPacket(void* opaque, uint8_t* buf, int32_t buf_size) {
		auto* reader = static_cast<IIoContext*>(opaque);
		if (!reader) {
			return AVERROR(EINVAL);
		}
		int32_t read = reader->Read(buf, buf_size);
		if (read < 0) {
			return AVERROR(EIO);
		}
		return read;
    }

    int32_t CustomWritePacket(void* opaque, uint8_t* buf, int32_t buf_size) {
        auto* writer = static_cast<IIoContext*>(opaque);
        if (!writer) {
            return AVERROR(EINVAL);
        }

        int32_t written = writer->Write(buf, buf_size);
        if (written < 0) {
            return AVERROR(EIO);
        }
        return written;
    }

    int64_t CustomSeekPacket(void* opaque, int64_t offset, int32_t whence) {
        auto* writer = static_cast<IIoContext*>(opaque);
        if (!writer) {
            return AVERROR(EINVAL);
        }
		return writer->Seek(offset, whence);
    }
}

class M4AFileEncoder::M4AFileEncoderImpl {
public:
    static constexpr auto kFrameSize = 1024;

    ~M4AFileEncoderImpl() {
        codec_context_.reset();
        output_io_context_.reset();
        format_context_.reset();
    }

    void Start(const AnyMap& config, const std::shared_ptr<IIoContext>& writer) {
        const auto input_file_path = config.AsPath(FileEncoderConfig::kInputFilePath);
        const auto output_file_path = config.AsPath(FileEncoderConfig::kOutputFilePath);
        codec_type_ = config.Get<std::string>(FileEncoderConfig::kCodecId);
        aac_bit_rate_ = config.Get<uint32_t>(FileEncoderConfig::kBitRate);
        file_name_ = String::ToUtf8String(output_file_path.wstring());
        input_file_ = StreamFactory::MakeFileStream(input_file_path, DsdModes::DSD_MODE_PCM);
        input_file_->OpenFile(input_file_path);
        writer_ = writer;
        CreateAudioStream();
    }

    void CreateAudioStream() {
        const auto channel_layout = AV_CH_LAYOUT_STEREO; // stereo
        std::string guess_file_name = "output.m4a";
        const char* file_name = file_name_.c_str();

        auto sample_format = AV_SAMPLE_FMT_NONE;

        const auto format = input_file_->GetFormat();
        AVCodecID codec_id = AV_CODEC_ID_NONE;

		// sample_size Only support ALAC encoding.
        uint32_t sample_size = 0;

        if (codec_type_ == "aac") {
            codec_id = AV_CODEC_ID_AAC;
            sample_format = AV_SAMPLE_FMT_FLTP;
            convert_ = [](const float* input, AVFrame* frame, size_t read_samples) {
                void* left_ch = frame->data[0];
                void* right_ch = frame->data[1];
				ConvertInterleavedToPlanarSSE(input,
					static_cast<float*>(left_ch),
					static_cast<float*>(right_ch),
                    read_samples / 2);
                };
		}
		else if (codec_type_ == "alac") {
			codec_id = AV_CODEC_ID_ALAC;
            if (input_file_->GetBitDepth() > 16) {
                sample_size = 24;
                sample_format = AV_SAMPLE_FMT_S32P;
                convert_ = [](const float* input, AVFrame* frame, size_t read_samples) {
                    void* left_ch = frame->data[0];
                    void* right_ch = frame->data[1];
                    ConvertFloatToInt24SSE(input,
                        static_cast<int32_t*>(left_ch),
                        static_cast<int32_t*>(right_ch),
                        read_samples / 2);
                    };
            }
            else {
                sample_format = AV_SAMPLE_FMT_S16P;
                sample_size = 16;
                convert_ = [](const float* input, AVFrame *frame, size_t read_samples) {
                    void* left_ch = frame->data[0];
                    void* right_ch = frame->data[1];
                    ConvertFloatToInt16SSE(input,
                        static_cast<int16_t*>(left_ch),
                        static_cast<int16_t*>(right_ch),
                        read_samples / 2);
                    };
            }
		}
        else if (codec_type_ == "pcm") {
            codec_id = AV_CODEC_ID_PCM_S16LE;
            sample_format = AV_SAMPLE_FMT_S16;
            sample_size = 16;
            guess_file_name = "output.wav";
            convert_ = [](const float* input, AVFrame* frame, size_t read_samples) {
                auto* dst = reinterpret_cast<int16_t*>(frame->data[0]);
                auto* src = input;

                for (int i = 0; i < read_samples; i += 2) {
                    float L = src[i + 0] * kFloat16Scale;
                    float R = src[i + 1] * kFloat16Scale;

                    dst[i + 0] = static_cast<int16_t>(L);
                    dst[i + 1] = static_cast<int16_t>(R);
                }
                };
        }

        if (writer_ != nullptr) {
            auto* avio_buffer = static_cast<uint8_t*>(LIBAV_LIB.Util->av_malloc(kFrameSize));
            if (!avio_buffer) {
                throw Exception("Fail to allocate AVIO buffer.");
            }

            auto* custom_io_ctx = LIBAV_LIB.Format->avio_alloc_context(
                avio_buffer,                // buffer
                kFrameSize,                 // buffer size
                1,                          // write_flag
                writer_.get(),              // opaque (指向IFileEncodeWriter)
                &CustomReadPacket,          // read_packet
                &CustomWritePacket,         // write_packet
                &CustomSeekPacket           // seek
            );
            if (!custom_io_ctx) {
                LIBAV_LIB.Util->av_free(avio_buffer);
                throw Exception("Fail to create custom AVIOContext.");
            }

            output_io_context_.reset(custom_io_ctx);

            format_context_.reset(LIBAV_LIB.Format->avformat_alloc_context());
            if (!format_context_) {
                throw Exception();
            }

            format_context_->pb = custom_io_ctx;
            format_context_->flags |= AVFMT_FLAG_CUSTOM_IO;
        } else {
            AVIOContext* output_io_context = nullptr;
            AvIfFailedThrow(LIBAV_LIB.Format->avio_open(&output_io_context, guess_file_name.c_str(), AVIO_FLAG_WRITE));
            output_io_context_.reset(output_io_context);

            format_context_.reset(LIBAV_LIB.Format->avformat_alloc_context());
            if (!format_context_) {
                throw Exception();
            }

            format_context_->pb = output_io_context;
        }

        auto* av_codec = LIBAV_LIB.Codec->avcodec_find_encoder(codec_id);
        if (!av_codec) {
            throw Exception("Encoder codec id not found.");
        }

        stream_ = LIBAV_LIB.Format->avformat_new_stream(format_context_.get(), av_codec);
        if (!stream_) {
            throw Exception("Failed to create new stream.");
        }

        codec_context_.reset(LIBAV_LIB.Codec->avcodec_alloc_context3(av_codec));
        codec_context_->channels = format.GetChannels();
        codec_context_->channel_layout = channel_layout;
        codec_context_->sample_rate = format.GetSampleRate();
        codec_context_->sample_fmt = sample_format;
        codec_context_->frame_size = kFrameSize;

        if (codec_type_ == "aac") {
			codec_context_->bit_rate = aac_bit_rate_;
			codec_context_->profile = FF_PROFILE_AAC_LOW;
        }

        stream_->id = format_context_->nb_streams - 1;
        stream_->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
        stream_->codecpar->codec_id = codec_id;
        stream_->codecpar->channel_layout = channel_layout;
        stream_->codecpar->channels = format.GetChannels();
        stream_->codecpar->sample_rate = format.GetSampleRate();
        stream_->codecpar->frame_size = kFrameSize;
        stream_->codecpar->format = sample_format;
        stream_->time_base = AVRational{ 1, static_cast<int32_t>(format.GetSampleRate()) };
        codec_context_->time_base = stream_->time_base;

        format_context_->oformat = LIBAV_LIB.Format->av_guess_format(nullptr, guess_file_name.c_str(), nullptr);

        if (format_context_->oformat->flags & AVFMT_GLOBALHEADER)
            codec_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        if (codec_type_ == "alac") {
            codec_context_->extradata = static_cast<uint8_t*>(LIBAV_LIB.Util->av_malloc(kAlacExtradataSize + AV_INPUT_BUFFER_PADDING_SIZE));
            codec_context_->extradata_size = kAlacExtradataSize;
            MemorySet(codec_context_->extradata, 0, kAlacExtradataSize);

            uint8_t* buf = codec_context_->extradata;
            auto frame_length = kFrameSize; // ensure consistent with frame_size
            auto avg_bitrate = format.GetSampleRate() * format.GetChannels() * sample_size; // bits per second
            auto sr = format.GetSampleRate();

            // Write alacSpecificConfig in big-endian
            bytestream_put_be32(&buf, frame_length);       // 4 bytes
            bytestream_put_byte(&buf, 0);            // compatibleVersion
            bytestream_put_byte(&buf, sample_size);        // bitDepth
            bytestream_put_byte(&buf, 40);           // pb
            bytestream_put_byte(&buf, 10);           // mb
            bytestream_put_byte(&buf, 14);           // kb (rice_param_limit)
            bytestream_put_byte(&buf, format.GetChannels()); // channels
            bytestream_put_be16(&buf, 255);          // maxRun
            bytestream_put_be32(&buf, 0);            // maxFrameBytes (0=unknown)
            bytestream_put_be32(&buf, avg_bitrate); 
            bytestream_put_be32(&buf, sr);

            // Padding to reach 36 bytes total
            for (int i = 0; i < 12; i++) {
                bytestream_put_byte(&buf, 0);
            }

            AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_open2(codec_context_.get(), av_codec, nullptr));
		}
		else if (codec_type_ == "aac") {
            AVDictionary* opts = nullptr;

            auto* codec = LIBAV_LIB.Codec->avcodec_find_encoder_by_name("libfdk_aac");
			if (!codec) {
                LIBAV_LIB.Util->av_dict_set(&opts, "profile", "aac_low", 0);
                AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_open2(codec_context_.get(), av_codec, &opts));
			}            
			else {
                LIBAV_LIB.Util->av_dict_set(&opts, "aac_object_type", "2", 0);
				AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_open2(codec_context_.get(), codec, &opts));
			}
		}

        AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_parameters_from_context(stream_->codecpar, codec_context_.get()));
        AvIfFailedThrow(LIBAV_LIB.Format->avformat_write_header(format_context_.get(), nullptr));
		LIBAV_LIB.Format->av_dump_format(format_context_.get(), 0, file_name, 1);
    }

    bool EncodeFrame(AVFrame* frame) {
        AvPtr<AVPacket> packet;
        packet.reset(LIBAV_LIB.Codec->av_packet_alloc());
		if (!packet) {
			throw Exception("Failed to allocate AVPacket.");
		}

        LIBAV_LIB.Codec->av_init_packet(packet.get());

        if (frame != nullptr) {
            frame->pts = LIBAV_LIB.Util->av_rescale_q(pts_, AVRational{ 1, frame->sample_rate }, codec_context_->time_base);
            pts_ += frame->nb_samples;
        }

        auto ret = LIBAV_LIB.Codec->avcodec_send_frame(codec_context_.get(), frame);
        if (ret == AVERROR_EOF) {
			XAMP_LOG_WARN("avcodec_send_frame: AVERROR_EOF");
			return false;
        }
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

    void Encode(const std::stop_token& stop_token, const std::function<bool(uint32_t)>& progress) {
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

        while (!stop_token.stop_requested()) {
            ret = LIBAV_LIB.Util->av_frame_make_writable(frame.get());
            if (ret < 0) {
                AvIfFailedThrow(ret);
            }

            const auto read_samples = input_file_->GetSamples(buffer_.data(),
                codec_context_->frame_size * 2);
            if (!read_samples) {
                break;
            }

            std::invoke(convert_, buffer_.data(), frame.get(), read_samples);

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
    uint32_t aac_bit_rate_{ 0 };
    int64_t pts_{ 0 };
    AVStream* stream_{ nullptr };
    std::string codec_type_;
    std::string file_name_;
    AvPtr<AVCodecContext> codec_context_;
    AvPtr<AVIOContext> output_io_context_;
    AvPtr<AVFormatContext> format_context_;
    std::function<void(const float*, AVFrame*, size_t)> convert_;
    std::shared_ptr<IIoContext> writer_;
    ScopedPtr<FileStream> input_file_;
    Buffer<float> buffer_;
};

M4AFileEncoder::M4AFileEncoder()
    : impl_(MakeAlign<M4AFileEncoderImpl>()) {
}

XAMP_PIMPL_IMPL(M4AFileEncoder)

void M4AFileEncoder::Start(const AnyMap& config, const std::shared_ptr<IIoContext>& io_context) {
    impl_->Start(config, io_context);
}

void M4AFileEncoder::Encode(const std::stop_token& stop_token, std::function<bool(uint32_t)> const& progress) {
    impl_->Encode(stop_token, progress);
}

XAMP_STREAM_NAMESPACE_END
