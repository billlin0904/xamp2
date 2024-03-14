#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/buffer.h>
#include <base/simd.h>
#include <fstream>

#include <stream/api.h>
#include <stream/alaclib.h>
#include <stream/filestream.h>
#include <stream/alacencoder.h>

XAMP_STREAM_NAMESPACE_BEGIN

class LibALACException : public Exception {
public:
    explicit LibALACException(int32_t error) {
    }
};

struct AlacFileEncoderTraits final {
    static void* invalid() {
        return nullptr;
    }

    static void close(void* value) {
        XAMP_EXPECTS(value != nullptr);
        ALAC_LIB.FinishEncoder(value);
    }
};

struct AudioDescription {
    double mSampleRate;
    uint32_t  mFormatID;
    uint32_t  mFormatFlags;
    uint32_t  mBytesPerPacket;
    uint32_t  mFramesPerPacket;
    uint32_t  mChannelsPerFrame;
    uint32_t  mBitsPerChannel;
};

enum {
#if XAMP_IS_BIG_ENDIAN
    kALACFormatFlagsNativeEndian = kALACFormatFlagIsBigEndian
#else
    kALACFormatFlagsNativeEndian = 0
#endif
};

enum {
    kCAFLinearPCMFormatFlagIsFloat = (1L << 0),
    kCAFLinearPCMFormatFlagIsLittleEndian = (1L << 1)
};

enum {
    kALACFormatAppleLossless = 'alac',
    kALACFormatLinearPCM = 'lpcm'
};

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
#if XAMP_IS_LITTLE_ENDIAN
    return BSWAP32(inUInt32);
#else
    return inUInt32;
#endif
}

void WriteCAFFcaffChunk(std::ofstream& file) {
    uint8_t buffer[8] = { 'c', 'a', 'f', 'f', 0, 1, 0, 0 };
    file.write((const char*)buffer, 8);
}

void WriteCAFFkukiChunk(std::ofstream& file, void* inCookie, uint32_t inCookieSize) {
    uint8_t buffer[12] = { 'k', 'u', 'k', 'i', 0, 0, 0, 0, 0, 0, 0, 0 };

    buffer[11] = inCookieSize;
    file.write((const char*)buffer, 12);
    file.write((const char *)inCookie, inCookieSize);
}

void WriteCAFFdescChunk(std::ofstream& file, const AudioDescription& output_format) {    
    AudioDescription description;
    uint32_t format_flags = output_format.mFormatFlags;
    uint8_t read_buffer[12] = { 'd', 'e', 's', 'c', 0, 0, 0, 0, 0, 0, 0, 0 };

    if (output_format.mFormatID == kALACFormatLinearPCM) {
        if (kALACFormatFlagsNativeEndian > 0) {
            format_flags = 0;
        } else {
            format_flags = kCAFLinearPCMFormatFlagIsLittleEndian;
        }
    }

    description.mSampleRate = SwapFloat64NtoB(output_format.mSampleRate);
    description.mFormatID = Swap32NtoB(output_format.mFormatID);
    description.mFormatFlags = Swap32NtoB(format_flags);
    description.mBytesPerPacket = Swap32NtoB(output_format.mBytesPerPacket);
    description.mFramesPerPacket = Swap32NtoB(output_format.mFramesPerPacket);
    description.mChannelsPerFrame = Swap32NtoB(output_format.mChannelsPerFrame);
    description.mBitsPerChannel = Swap32NtoB(output_format.mBitsPerChannel);

    read_buffer[11] = sizeof(AudioDescription);
    file.write((const char *)read_buffer, 12);
    file.write((const char*) &description, sizeof(AudioDescription));
}

using AlacFileEncoderHandle = UniqueHandle<void*, AlacFileEncoderTraits>;

class AlacFileEncoder::AlacFileEncoderImpl {
public:
    static constexpr int32_t kFramesPerPacket = 4096;

    void Start(const AnyMap& config) {
        const auto input_file_path = config.AsPath(FileEncoderConfig::kInputFilePath);
        const auto output_file_path = config.AsPath(FileEncoderConfig::kOutputFilePath);
        input_file_ = StreamFactory::MakeFileStream(DsdModes::DSD_MODE_PCM, input_file_path);
        output_file_.open(output_file_path, std::ios::binary);
        input_file_->OpenFile(input_file_path);
        auto format = input_file_->GetFormat();
        input_buffer_.resize(kFramesPerPacket);
        output_buffer_.resize(kFramesPerPacket);
        handle_.reset(ALAC_LIB.InitializeEncoder(format.GetSampleRate(),
            format.GetChannels(),
            format.GetBitsPerSample(),
            kFramesPerPacket,
            false));
    }

    void Encode(std::function<bool(uint32_t)> const& progress) {
        while (true) {
            auto read_samples = input_file_->GetSamples(input_buffer_.data(), input_buffer_.size());
            if (!read_samples) {
                break;
            }

            for (auto i = 0; i < read_samples; ++i) {
                convert_buffer_[i] = static_cast<int16_t>(kFloat16Scale * input_buffer_[i]);
            }

            int ret = output_buffer_.size();
            if (ALAC_LIB.Encode(handle_.get(), (unsigned char *)convert_buffer_.data(), output_buffer_.data(),
                &ret) != 0) {
                throw LibALACException(ret);
            }

            if (ret > 0) {
                if (!output_file_.write((const char*)output_buffer_.data(), ret)) {
                    break;
                }
            }
        }
    }
private:        
    Buffer<float> input_buffer_;
    Buffer<int16_t> convert_buffer_;
    Buffer<uint8_t> output_buffer_;
    AlignPtr<FileStream> input_file_;
    std::ofstream output_file_;
    AlacFileEncoderHandle handle_;
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
