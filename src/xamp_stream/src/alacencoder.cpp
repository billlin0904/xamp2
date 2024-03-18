#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/buffer.h>
#include <base/simd.h>
#include <fstream>

#include <stream/avlib.h>
#include <stream/api.h>
#include <stream/filestream.h>
#include <stream/alacencoder.h>

XAMP_STREAM_NAMESPACE_BEGIN

namespace {
#define BSWAP16(x) (((x << 8) | ((x >> 8) & 0x00ff)))
#define BSWAP32(x) (((x << 24) | ((x << 8) & 0x00ff0000) | ((x >> 8) & 0x0000ff00) | ((x >> 24) & 0x000000ff)))
#define BSWAP64(x) ((((int64_t)x << 56) | (((int64_t)x << 40) & 0x00ff000000000000LL) | \
                    (((int64_t)x << 24) & 0x0000ff0000000000LL) | (((int64_t)x << 8) & 0x000000ff00000000LL) | \
                    (((int64_t)x >> 8) & 0x00000000ff000000LL) | (((int64_t)x >> 24) & 0x0000000000ff0000LL) | \
                    (((int64_t)x >> 40) & 0x000000000000ff00LL) | (((int64_t)x >> 56) & 0x00000000000000ffLL)))

    double SwapFloat64NtoB(double in) {
#if XAMP_IS_LITTLE_ENDIAN
        union {
            double f;
            int64_t i;
        } x;
        x.f = in;
        x.i = BSWAP64(x.i);
        return x.f;
#else
        return in;
#endif
    }

    uint32_t Swap32NtoB(uint32_t inUInt32) {
#if TARGET_RT_LITTLE_ENDIAN
        return BSWAP32(inUInt32);
#else
        return inUInt32;
#endif
    }

    struct CAFAudioDescription {
        double mSampleRate;
        uint32_t  mFormatID;
        uint32_t  mFormatFlags;
        uint32_t  mBytesPerPacket;
        uint32_t  mFramesPerPacket;
        uint32_t  mChannelsPerFrame;
        uint32_t  mBitsPerChannel;
    };

    enum {
        kALACFormatAppleLossless = 'alac',
        kALACFormatLinearPCM = 'lpcm'
    };

    enum {
        kALACFormatFlagIsFloat = (1 << 0),     // 0x1
        kALACFormatFlagIsBigEndian = (1 << 1),     // 0x2
        kALACFormatFlagIsSignedInteger = (1 << 2),     // 0x4
        kALACFormatFlagIsPacked = (1 << 3),     // 0x8
        kALACFormatFlagIsAlignedHigh = (1 << 4),     // 0x10
    };
}

class AlacFileEncoder::AlacFileEncoderImpl {
public:
    void Start(const AnyMap& config) {
        const auto input_file_path = config.AsPath(FileEncoderConfig::kInputFilePath);
        const auto output_file_path = config.AsPath(FileEncoderConfig::kOutputFilePath);
        file_name_ = output_file_path.string();
        input_file_ = StreamFactory::MakeFileStream(DsdModes::DSD_MODE_PCM, input_file_path);
        input_file_->OpenFile(input_file_path);
        CreateAudioSteam();
    }

    void CreateAudioSteam() {
        const char file_name[] = "output.m4a";
        AVIOContext* output_io_context = nullptr;
        AvIfFailedThrow(LIBAV_LIB.Format->avio_open(&output_io_context, file_name, AVIO_FLAG_WRITE));

        format_context_.reset(LIBAV_LIB.Format->avformat_alloc_context());
        format_context_->pb = output_io_context;
        strcpy_s(format_context_->filename, sizeof(format_context_->filename), file_name);

        auto * av_codec = LIBAV_LIB.Codec->avcodec_find_decoder(AV_CODEC_ID_ALAC);
        stream_.reset(LIBAV_LIB.Format->avformat_new_stream(format_context_.get(), av_codec));

        const auto best_sample_format = AV_SAMPLE_FMT_S32P;//FindBestMatchingSampleFormat(AV_SAMPLE_FMT_FLT, av_codec->sample_fmts);
        const auto format = input_file_->GetFormat();

        codec_context_.reset(LIBAV_LIB.Codec->avcodec_alloc_context3(av_codec));

        stream_->id = format_context_->nb_streams - 1;
        stream_->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
        stream_->codecpar->codec_id = AV_CODEC_ID_ALAC;
        stream_->codecpar->channel_layout = 3;
        stream_->codecpar->channels = format.GetChannels();
        stream_->codecpar->sample_rate = format.GetSampleRate();
        stream_->codecpar->frame_size = 1024;
        stream_->codecpar->format = best_sample_format;
        stream_->time_base = AVRational{ 1, static_cast<int32_t>(format.GetSampleRate()) };
        codec_context_->time_base = stream_->time_base;

        AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_parameters_from_context(stream_->codecpar, codec_context_.get()));

        format_context_->oformat = LIBAV_LIB.Format->av_guess_format(nullptr, file_name, nullptr);
        if (format_context_->oformat->flags & AVFMT_GLOBALHEADER)
            codec_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

#define ALAC_EXTRADATA_SIZE 36

        codec_context_->extradata = static_cast<uint8_t*>(av_malloc(ALAC_EXTRADATA_SIZE));
        codec_context_->extradata_size = ALAC_EXTRADATA_SIZE;
        MemorySet(codec_context_->extradata, 0, ALAC_EXTRADATA_SIZE);

        // Skip size:4, alac:4, version:4 = 12
        auto buffer = codec_context_->extradata + 12;

        CAFAudioDescription desc{};
        desc.mSampleRate       = SwapFloat64NtoB(format.GetSampleRate());
        desc.mFormatID         = Swap32NtoB(kALACFormatLinearPCM);
        desc.mFormatFlags      = Swap32NtoB(kALACFormatFlagIsSignedInteger | kALACFormatFlagIsPacked);
        desc.mChannelsPerFrame = Swap32NtoB(format.GetChannels());
        desc.mBitsPerChannel   = Swap32NtoB(32);
        desc.mFramesPerPacket  = Swap32NtoB((32 >> 3) * format.GetChannels());
        desc.mBytesPerPacket   = desc.mFramesPerPacket;
        //MemoryCopy(buffer, &desc, sizeof(desc));
        uint32_t max_samples_per_frame = 1024;
        MemoryCopy(buffer, &max_samples_per_frame, sizeof(max_samples_per_frame));
        buffer += 5; // compatible version
        uint8_t sample_size = 32;
        MemoryCopy(buffer, &sample_size, sizeof(sample_size));

        stream_->codecpar->extradata = static_cast<uint8_t*>(av_malloc(codec_context_->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE));
        stream_->codecpar->extradata_size = codec_context_->extradata_size;
        MemorySet(stream_->codecpar->extradata, 0, stream_->codecpar->extradata_size);

        MemoryCopy(stream_->codecpar->extradata, codec_context_->extradata, codec_context_->extradata_size);

        AvIfFailedThrow(LIBAV_LIB.Codec->avcodec_open2(codec_context_.get(), av_codec, nullptr));
        
        AvIfFailedThrow(LIBAV_LIB.Format->avio_open(&format_context_->pb, file_name_.c_str(), AVIO_FLAG_WRITE));
        AvIfFailedThrow(LIBAV_LIB.Format->avformat_write_header(format_context_.get(), nullptr));
    }

    void Encode(std::function<bool(uint32_t)> const& progress) {
        
    }
private:
    AvPtr<AVCodecContext> codec_context_;
    AvPtr<AVFormatContext> format_context_;
    AvPtr<AVStream> stream_;
    AlignPtr<FileStream> input_file_;
    std::string file_name_;
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
