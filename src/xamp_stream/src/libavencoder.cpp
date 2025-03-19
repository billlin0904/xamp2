#include <immintrin.h>

#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/buffer.h>
#include <base/dataconverter.h>
#include <base/port.h>

#include <stream/avlib.h>
#include <stream/api.h>
#include <stream/icddevice.h>
#include <stream/filestream.h>
#include <stream/libavencoder.h>
#include <stream/bassfilestream.h>

XAMP_STREAM_NAMESPACE_BEGIN

namespace {
    // ALAC extradata size is 36 bytes
    constexpr int kAlacExtradataSize = 36;

    // 實作幾個工具函式，用來向 uint8_t* buffer 寫入 big-endian 格式資料
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
}

class LibAbFileEncoder::LibAbFileEncoderImpl {
public:
    static constexpr auto kDefaultChannelLayout = AV_CH_LAYOUT_STEREO;
    static constexpr auto kFrameSize = 8192;

    ~LibAbFileEncoderImpl() {
        // 確保在解構時釋放重要資源
        codec_context_.reset();
        output_io_context_.reset();
        format_context_.reset();
    }

    void Start(const AnyMap& config, const std::shared_ptr<IFile>& writer) {
        // 1) 從 config 解析關鍵參數
        const auto input_file_path = config.AsPath(FileEncoderConfig::kInputFilePath);
        const auto output_file_path = config.AsPath(FileEncoderConfig::kOutputFilePath);
        codec_type_ = config.Get<std::string>(FileEncoderConfig::kCodecId);
        aac_bit_rate_ = config.Get<uint32_t>(FileEncoderConfig::kBitRate);
        file_name_ = String::ToUtf8String(output_file_path.wstring());

        // 3) 建立輸入檔案讀取物件並打開
        input_file_ = StreamFactory::MakeFileStream(input_file_path);
        input_file_->OpenFile(input_file_path);

        // 4) 若未指定 writer，則建立檔案寫入物件
        if (!writer) {
            writer_ = MakFileEncodeWriter(output_file_path);
        }
        else {
            writer_ = writer;
        }

        // 5) 建立音訊串流並初始化編碼相關參數
        CreateAudioStream();
    }

    void CreateAudioStream() {
        // 預設輸出檔名 (若未指定 writer 或 container)
        std::string guess_file_name = "output.m4a";
        const char* file_name = file_name_.c_str();

        auto sample_format = AV_SAMPLE_FMT_NONE;
        const auto format = input_file_->GetFormat();
        AVCodecID codec_id = AV_CODEC_ID_NONE;
        uint32_t sample_size = 0; // bit depth: 16 / 24 ...

        //----------------------------------------------------------------------
        // 根據 codec_type_ (aac / alac / pcm) 選擇對應的 codec_id、sample_fmt、bit depth
        //----------------------------------------------------------------------

        if (codec_type_ == "aac") {
            sample_size = 16;
            codec_id = AV_CODEC_ID_AAC;
            sample_format = AV_SAMPLE_FMT_FLTP;

            // for AAC: convert float => float planar
            convert_ = [](const float* input, AVFrame* frame, size_t read_samples) {
                void* left_ch = frame->data[0];
                void* right_ch = frame->data[1];

                // SSE convert: interleaved float => planar float
                ConvertFloatToFloatSSE(input,
                    static_cast<float*>(left_ch),
                    static_cast<float*>(right_ch),
                    read_samples / 2);
                };
        }
        else if (codec_type_ == "alac") {
            codec_id = AV_CODEC_ID_ALAC;

            // 若 bit depth > 16 => 24-bit
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
                sample_size = 16;
                sample_format = AV_SAMPLE_FMT_S16P;
                convert_ = [](const float* input, AVFrame* frame, size_t read_samples) {
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
            // 預設產生 WAV 檔
            guess_file_name = "output.wav";

            // 判斷 bit depth，若 >16 => 24-bit
            if (input_file_->GetBitDepth() > 16) {
                codec_id = AV_CODEC_ID_PCM_S24LE;
                sample_size = 24;
                sample_format = AV_SAMPLE_FMT_S32; // pack 24bit in 32
                convert_ = [](const float* input, AVFrame* frame, size_t read_samples) {
                    AudioConvertContext ctx;
                    ctx.convert_size = read_samples / 2;
                    // SSE convert: float => 24-bit (packed in 32bit)
                    DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::Convert(
                        reinterpret_cast<int32_t*>(frame->data[0]), input, ctx);
                    };
            }
            else {
                codec_id = AV_CODEC_ID_PCM_S16LE;
                sample_size = 16;
                sample_format = AV_SAMPLE_FMT_S16;
                convert_ = [](const float* input, AVFrame* frame, size_t read_samples) {
                    AudioConvertContext ctx;
                    ctx.convert_size = read_samples / 2;
                    // SSE convert: float => 16-bit
                    DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::Convert(
                        reinterpret_cast<int16_t*>(frame->data[0]), input, ctx);
                    };
            }
        }

        //----------------------------------------------------------------------
        // 建立輸出 AVFormatContext + AVIOContext (若使用自訂 IO)
        //----------------------------------------------------------------------

        if (writer_ != nullptr) {
            // 使用自訂 I/O
            auto* avio_buffer = static_cast<uint8_t*>(LIBAV_LIB.Util->av_malloc(kFrameSize));
            if (!avio_buffer) {
                throw Exception("Fail to allocate AVIO buffer.");
            }

            auto* custom_io_ctx = LIBAV_LIB.Format->avio_alloc_context(
                avio_buffer,                // buffer
                kFrameSize,                 // buffer size
                1,                          // write_flag
                writer_.get(),              // opaque (指向 IFileEncodeWriter)
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

            // 指定自訂 pb，並標示它是 custom_io
            format_context_->pb = custom_io_ctx;
            format_context_->flags |= AVFMT_FLAG_CUSTOM_IO;
        }
        else {
            // 直接開檔案寫
            AVIOContext* output_io_context = nullptr;
            AvIfFailedThrow(LIBAV_LIB.Format->avio_open(
                &output_io_context,
                guess_file_name.c_str(),
                AVIO_FLAG_WRITE
            ));
            output_io_context_.reset(output_io_context);

            format_context_.reset(LIBAV_LIB.Format->avformat_alloc_context());
            if (!format_context_) {
                throw Exception();
            }

            format_context_->pb = output_io_context;
        }

        // 猜測輸出封裝格式
        format_context_->oformat = LIBAV_LIB.Format->av_guess_format(
            nullptr, guess_file_name.c_str(), nullptr
        );

        //----------------------------------------------------------------------
        // 依據 codec_id 建立 encoder，並分配 AVCodecContext
        //----------------------------------------------------------------------

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
        codec_context_->channel_layout = kDefaultChannelLayout;
        codec_context_->sample_rate = format.GetSampleRate();
        codec_context_->sample_fmt = sample_format;
        codec_context_->bits_per_raw_sample = sample_size;

        // 壓縮編碼通常有固定 frame_size，PCM 設為 0 由呼叫端自行控制
        if (codec_type_ != "pcm") {
            codec_context_->frame_size = kFrameSize;
        }
        else {
            codec_context_->frame_size = 0;
        }

        // 若為 AAC，指定位元率與 profile
        if (codec_type_ == "aac") {
            codec_context_->bit_rate = aac_bit_rate_;
            codec_context_->profile = FF_PROFILE_AAC_LOW;
        }

        // stream_ 的 codecpar 初始化
        stream_->id = format_context_->nb_streams - 1;
        stream_->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
        stream_->codecpar->codec_id = codec_id;
        stream_->codecpar->channel_layout = kDefaultChannelLayout;
        stream_->codecpar->channels = format.GetChannels();
        stream_->codecpar->sample_rate = format.GetSampleRate();
        stream_->codecpar->format = sample_format;

        if (codec_type_ != "pcm") {
            stream_->codecpar->frame_size = kFrameSize;
        }
        else {
            stream_->codecpar->frame_size = 0;
        }

        // 時基 (time_base) 設定
        stream_->time_base = AVRational{ 1, static_cast<int32_t>(format.GetSampleRate()) };
        codec_context_->time_base = stream_->time_base;

        // 如果封裝需要全域 header (如 MP4/M4A)
        if (format_context_->oformat->flags & AVFMT_GLOBALHEADER) {
            codec_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        //----------------------------------------------------------------------
        // 若為 ALAC，需要手動填入 36 bytes extradata (ALACSpecificConfig)
        //----------------------------------------------------------------------

        if (codec_type_ == "alac") {
            codec_context_->extradata = static_cast<uint8_t*>(
                LIBAV_LIB.Util->av_malloc(kAlacExtradataSize + AV_INPUT_BUFFER_PADDING_SIZE));
            codec_context_->extradata_size = kAlacExtradataSize;
            MemorySet(codec_context_->extradata, 0, kAlacExtradataSize);

            uint8_t* buf = codec_context_->extradata;
            auto frame_length = kFrameSize;
            auto avg_bitrate = format.GetSampleRate() * format.GetChannels() * sample_size;
            auto sr = format.GetSampleRate();

            // 以 big-endian 寫入 ALAC specific config
            bytestream_put_be32(&buf, frame_length); // 4 bytes
            bytestream_put_byte(&buf, 0);            // compatibleVersion
            bytestream_put_byte(&buf, sample_size);  // bitDepth
            bytestream_put_byte(&buf, 40);           // pb
            bytestream_put_byte(&buf, 10);           // mb
            bytestream_put_byte(&buf, 14);           // kb (rice_param_limit)
            bytestream_put_byte(&buf, format.GetChannels()); // channels
            bytestream_put_be16(&buf, 255);          // maxRun
            bytestream_put_be32(&buf, 0);            // maxFrameBytes (0=unknown)
            bytestream_put_be32(&buf, avg_bitrate);
            bytestream_put_be32(&buf, sr);

            // 填至 36 bytes
            for (int i = 0; i < 12; i++) {
                bytestream_put_byte(&buf, 0);
            }
            // 避免使用 compression_level = 0

            // 開啟 ALAC 編碼器
            AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_open2(codec_context_.get(), av_codec, nullptr));
        }
        else if (codec_type_ == "aac") {
            // AAC 需要指定 profile
            AVDictionary* opts = nullptr;
            LIBAV_LIB.Util->av_dict_set(&opts, "profile", "aac_low", 0);

            AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_open2(codec_context_.get(), av_codec, &opts));
        }
        else if (codec_type_ == "pcm") {
            // PCM 也要開啟編碼器流程
            AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_open2(codec_context_.get(), av_codec, nullptr));
        }

        // 將 codec_context 參數複製到 stream_->codecpar
        AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_parameters_from_context(
            stream_->codecpar, codec_context_.get()));

        // 寫入容器標頭
        AvIfFailedThrow(LIBAV_LIB.Format->avformat_write_header(
            format_context_.get(), nullptr));

        // dump 格式資訊
        LIBAV_LIB.Format->av_dump_format(format_context_.get(), 0, file_name, 1);
    }

    bool EncodeFrame(AVFrame* frame) {
        // 建立並初始化 AVPacket
        AvPtr<AVPacket> packet;
        packet.reset(LIBAV_LIB.Codec->av_packet_alloc());
        if (!packet) {
            throw Exception("Failed to allocate AVPacket.");
        }
        LIBAV_LIB.Codec->av_init_packet(packet.get());

        // 設定 frame->pts
        if (frame != nullptr) {
            frame->pts = LIBAV_LIB.Util->av_rescale_q(
                pts_,
                AVRational{ 1, frame->sample_rate },
                codec_context_->time_base
            );
            pts_ += frame->nb_samples;
        }

        // 送入 frame 給編碼器
        auto ret = LIBAV_LIB.Codec->avcodec_send_frame(codec_context_.get(), frame);
        if (ret == AVERROR_EOF) {
            XAMP_LOG_WARN("avcodec_send_frame: AVERROR_EOF");
            return false;
        }
        AvIfFailedThrow(ret);

        // 從編碼器讀取封包
        ret = LIBAV_LIB.Codec->avcodec_receive_packet(codec_context_.get(), packet.get());
        if (ret == AVERROR_EOF) {
            return false;
        }
        if (ret == AVERROR(EAGAIN)) {
            // 尚無可用封包，需等待更多輸入
            return true;
        }
        else {
            AvIfFailedThrow(ret);
        }

        // 寫入封裝容器
        ret = LIBAV_LIB.Format->av_interleaved_write_frame(format_context_.get(), packet.get());
        AvIfFailedThrow(ret);
        return true;
    }

    void Encode(const std::function<bool(uint32_t)>& progress,
        const std::stop_token& stop_token) {
        // 分配並初始化輸出用的 AVFrame
        AvPtr<AVFrame> frame;
        frame.reset(LIBAV_LIB.Util->av_frame_alloc());
        frame->format = codec_context_->sample_fmt;
        frame->channel_layout = codec_context_->channel_layout;
        frame->sample_rate = codec_context_->sample_rate;

        // 若編碼器指定了 frame_size，就用它；否則 PCM 編碼由我們自行決定
        if (codec_context_->frame_size > 0) {
            frame->nb_samples = codec_context_->frame_size;
            buffer_.resize(codec_context_->frame_size * 2);
        }
        else {
            frame->nb_samples = kFrameSize;
            buffer_.resize(kFrameSize * 2);
        }

        // 預估總樣本數以便顯示進度
        auto format = input_file_->GetFormat();
        uint64_t total_samples = static_cast<uint64_t>(
            input_file_->GetDurationAsSeconds()
            * format.GetSampleRate()
            * format.GetChannels()
            );
        uint64_t processed_samples = 0;

        // 要先為 frame 分配 buffer
        auto ret = LIBAV_LIB.Util->av_frame_get_buffer(frame.get(), 0);
        if (ret < 0) {
            AvIfFailedThrow(ret);
        }

        uint64_t percentage = 0;
        uint64_t read_samples = 0;
        uint64_t retry_count = 0;

        //--------------------------------------------------------------------------
        // 主迴圈：反覆從 input_file_ 讀取資料 -> 轉換 -> EncodeFrame()
        //--------------------------------------------------------------------------

        auto bass_file_stream = dynamic_cast<BassFileStream*>(input_file_.get());
		constexpr auto kWaitCDReadTime = std::chrono::milliseconds(100);

        while (!stop_token.stop_requested() && input_file_->IsActive()) {
            // 確保 frame 可寫
            ret = LIBAV_LIB.Util->av_frame_make_writable(frame.get());
            AvIfFailedThrow(ret);

            read_samples = 0;
            buffer_.Fill(0.0f);

            // 讀取樣本，若一時讀不到就重試 (最多4次)，中間sleep 100ms
            if (bass_file_stream != nullptr) {
                while (read_samples == 0 && retry_count < 4) {
                    read_samples = input_file_->GetSamples(buffer_.data(), buffer_.GetSize());
                    if (read_samples == 0 && !bass_file_stream->EndOfStream()) {
                        std::this_thread::sleep_for(kWaitCDReadTime);
                        retry_count++;
                    }
                    else {
                        break;
                    }
                }
            }
            else {
                read_samples = input_file_->GetSamples(buffer_.data(), buffer_.GetSize());
            }            

            if (!read_samples) {
                break; // 沒資料可讀，跳出
            }

            retry_count = 0;

            // 呼叫 convert_ 把 float PCM -> 對應的整數或浮點格式 (依 codec_type_ 分支而定)
            std::invoke(convert_, buffer_.data(), frame.get(), read_samples);

            // 送入編碼器
            if (!EncodeFrame(frame.get())) {
                break;
            }

            // 更新進度
            processed_samples += read_samples;
            if (total_samples > 0) {
                percentage = static_cast<uint32_t>(
                    (static_cast<double>(processed_samples)
                        / static_cast<double>(total_samples)) * 100
                    );
            }
            if (!progress(percentage)) {
                break; // 若外部回呼要取消，就跳出
            }
        }

        // flush：送入 nullptr frame
        while (EncodeFrame(nullptr)) {
            // 直到 EncodeFrame return false 或 avcodec_receive_packet() 完成
        }

        // 寫入容器尾資訊
        AvIfFailedThrow(LIBAV_LIB.Format->av_write_trailer(format_context_.get()));
    }

private:
    //--------------------------------------------------------------------------
    // Custom I/O callback functions
    //--------------------------------------------------------------------------

    static int32_t CustomReadPacket(void* opaque, uint8_t* buf, int32_t buf_size) {
        auto* reader = static_cast<IFile*>(opaque);
        if (!reader) {
            return AVERROR(EINVAL);
        }
        int32_t read = reader->Read(buf, buf_size);
        if (read < 0) {
            return AVERROR(EIO);
        }
        return read;
    }

    static int32_t CustomWritePacket(void* opaque, uint8_t* buf, int32_t buf_size) {
        auto* writer = static_cast<IFile*>(opaque);
        if (!writer) {
            return AVERROR(EINVAL);
        }
        int32_t written = writer->Write(buf, buf_size);
        if (written < 0) {
            return AVERROR(EIO);
        }
        return written;
    }

    static int64_t CustomSeekPacket(void* opaque, int64_t offset, int32_t whence) {
        auto* writer = static_cast<IFile*>(opaque);
        if (!writer) {
            return AVERROR(EINVAL);
        }
        return writer->Seek(offset, whence);
    }

private:
    //--------------------------------------------------------------------------
    // 成員變數
    //--------------------------------------------------------------------------
    uint32_t aac_bit_rate_{ 0 };
    int64_t pts_{ 0 };
    AVStream* stream_{ nullptr };
    std::string codec_type_;
    std::string file_name_;

    AvPtr<AVCodecContext> codec_context_;
    AvPtr<AVIOContext> output_io_context_;
    AvPtr<AVFormatContext> format_context_;

    // 用於 float PCM => 對應格式的轉換函式
    std::function<void(const float*, AVFrame*, size_t)> convert_;

    std::shared_ptr<IFile> writer_;
    ScopedPtr<FileStream> input_file_;
    Buffer<float> buffer_;
};

//------------------------------------------------------------------------------
// LibAbFileEncoder 實作
//------------------------------------------------------------------------------
LibAbFileEncoder::LibAbFileEncoder()
    : impl_(MakeAlign<LibAbFileEncoderImpl>()) {
}

XAMP_PIMPL_IMPL(LibAbFileEncoder)

void LibAbFileEncoder::Start(const AnyMap& config, const std::shared_ptr<IFile>& file) {
    impl_->Start(config, file);
}

void LibAbFileEncoder::Encode(std::function<bool(uint32_t)> const& progress,
    const std::stop_token& stop_token) {
    impl_->Encode(progress, stop_token);
}

XAMP_STREAM_NAMESPACE_END
