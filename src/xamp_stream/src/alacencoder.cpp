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

struct AudioFormatDescription {
    double mSampleRate;
    uint32_t  mFormatID;
    uint32_t  mFormatFlags;
    uint32_t  mBytesPerPacket;
    uint32_t  mFramesPerPacket;
    uint32_t  mBytesPerFrame;
    uint32_t  mChannelsPerFrame;
    uint32_t  mBitsPerChannel;
    uint32_t  mReserved;
};

struct CAFPacketTableHeader {
    int64_t  mNumberPackets;
    int64_t  mNumberValidFrames;
    int32_t  mPrimingFrames;
    int32_t  mRemainderFrames;

    uint8_t   mPacketDescriptions[1]; // this is a variable length array of mNumberPackets elements
};

constexpr auto kMinCAFFPacketTableHeaderSize = 24;

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

enum {
    kALACMaxChannels = 8,
    kALACMaxEscapeHeaderBytes = 8,
    kALACMaxSearches = 16,
    kALACMaxCoefs = 16,
    kALACDefaultFramesPerPacket = 4096
};

enum {
    kTestFormatFlag_16BitSourceData = 1,
    kTestFormatFlag_20BitSourceData = 2,
    kTestFormatFlag_24BitSourceData = 3,
    kTestFormatFlag_32BitSourceData = 4
};


#define BSWAP32(x) (((x << 24) | ((x << 8) & 0x00ff0000) | ((x >> 8) & 0x0000ff00) | ((x >> 24) & 0x000000ff)))
#define BSWAP64(x) ((((int64_t)x << 56) | (((int64_t)x << 40) & 0x00ff000000000000LL) | \
                    (((int64_t)x << 24) & 0x0000ff0000000000LL) | (((int64_t)x << 8) & 0x000000ff00000000LL) | \
                    (((int64_t)x >> 8) & 0x00000000ff000000LL) | (((int64_t)x >> 24) & 0x0000000000ff0000LL) | \
                    (((int64_t)x >> 40) & 0x000000000000ff00LL) | (((int64_t)x >> 56) & 0x00000000000000ffLL)))

uint64_t Swap64NtoB(uint64_t inUInt64) {
#if XAMP_IS_LITTLE_ENDIAN
    return BSWAP64(inUInt64);
#else
    return inUInt64;
#endif
}

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

class CAFFFileWriter {
public:
	explicit CAFFFileWriter(const Path& file_path) {
        file_.open(file_path, std::ios::binary);
    }

    void WriteCAFFdataChunk() {
        uint8_t buffer[16] = { 'd', 'a', 't', 'a', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
        file_.write(reinterpret_cast<const char*>(buffer), 16);
	}

    void WriteCAFFcaffChunk() {
	    const uint8_t buffer[8] = { 'c', 'a', 'f', 'f', 0, 1, 0, 0 };
        file_.write(reinterpret_cast<const char*>(buffer), 8);
    }

    void WriteCAFFkukiChunk(void* cookie, uint32_t cookie_size) {
        uint8_t buffer[12] = { 'k', 'u', 'k', 'i', 0, 0, 0, 0, 0, 0, 0, 0 };

        buffer[11] = cookie_size;
        file_.write(reinterpret_cast<const char*>(buffer), 12);
        file_.write(reinterpret_cast<const char*>(&cookie), cookie_size);
    }

    void WriteCAFFdescChunk(const AudioFormatDescription& output_format) {
        AudioFormatDescription description;
        uint32_t format_flags = output_format.mFormatFlags;
        uint8_t read_buffer[12] = { 'd', 'e', 's', 'c', 0, 0, 0, 0, 0, 0, 0, 0 };

        if (output_format.mFormatID == kALACFormatLinearPCM) {
            if (kALACFormatFlagsNativeEndian > 0) {
                format_flags = 0;
            }
            else {
                format_flags = kCAFLinearPCMFormatFlagIsLittleEndian;
            }
        }

        description.mSampleRate       = SwapFloat64NtoB(output_format.mSampleRate);
        description.mFormatID         = Swap32NtoB(output_format.mFormatID);
        description.mFormatFlags      = Swap32NtoB(format_flags);
        description.mBytesPerPacket   = Swap32NtoB(output_format.mBytesPerPacket);
        description.mFramesPerPacket  = Swap32NtoB(output_format.mFramesPerPacket);
        description.mChannelsPerFrame = Swap32NtoB(output_format.mChannelsPerFrame);
        description.mBitsPerChannel   = Swap32NtoB(output_format.mBitsPerChannel);

        read_buffer[11] = sizeof(AudioFormatDescription);
        file_.write(reinterpret_cast<const char*>(read_buffer), 12);
        file_.write(reinterpret_cast<const char*>(&description), sizeof(AudioFormatDescription));
    }

	int32_t GetPosition() {
        return file_.tellp();
	}

    void WriteCAFFpaktChunkHeader(CAFPacketTableHeader* packet_table_header, uint32_t packet_table_size) {        
        packet_table_header->mNumberPackets     = Swap64NtoB(packet_table_header->mNumberPackets);
        packet_table_header->mNumberValidFrames = Swap64NtoB(packet_table_header->mNumberValidFrames);
        packet_table_header->mPrimingFrames     = Swap32NtoB(packet_table_header->mPrimingFrames);
        packet_table_header->mRemainderFrames   = Swap32NtoB(packet_table_header->mRemainderFrames);

    	uint8_t buffer[12];
        buffer[0] = 'p';
        buffer[1] = 'a';
        buffer[2] = 'k';
        buffer[3] = 't';
        buffer[4] = 0;
        buffer[5] = 0;
        buffer[6] = 0;
        buffer[7] = 0;
        buffer[8] = packet_table_size >> 24;
        buffer[9] = (packet_table_size >> 16) & 0xff;
        buffer[10] = (packet_table_size >> 8) & 0xff;
        buffer[11] = packet_table_size & 0xff;

        file_.write(reinterpret_cast<const char*>(buffer), 12);
        file_.write(reinterpret_cast<const char*>(packet_table_header), kMinCAFFPacketTableHeaderSize);
    }

    void WritePacketTableEntries(const std::vector<uint8_t> &packet_table_entries) {
        file_.write(reinterpret_cast<const char*>(packet_table_entries.data()), packet_table_entries.size());
	}

    static int32_t BuildBasePacketTable(const AudioFormatDescription& input_format, int32_t input_data_size, int32_t* max_packet_table_size, CAFPacketTableHeader* packet_table_header) {
        int32_t max_packet_size = 0, byte_size_table_entry = 0;

        // fill out the header
        packet_table_header->mNumberValidFrames = input_data_size / ((input_format.mBitsPerChannel >> 3) * input_format.mChannelsPerFrame);
        packet_table_header->mNumberPackets = packet_table_header->mNumberValidFrames / kALACDefaultFramesPerPacket;
        packet_table_header->mPrimingFrames = 0;
        packet_table_header->mRemainderFrames = packet_table_header->mNumberValidFrames - packet_table_header->mNumberPackets * kALACDefaultFramesPerPacket;
        packet_table_header->mRemainderFrames = kALACDefaultFramesPerPacket - packet_table_header->mRemainderFrames;
        if (packet_table_header->mRemainderFrames) 
            packet_table_header->mNumberPackets += 1;

        // Ok, we have to assume the worst case scenario for packet sizes
        max_packet_size = (input_format.mBitsPerChannel >> 3) * input_format.mChannelsPerFrame * kALACDefaultFramesPerPacket + kALACMaxEscapeHeaderBytes;

        if (max_packet_size < 16384) {
            byte_size_table_entry = 2;
        } else {
            byte_size_table_entry = 3;
        }

        *max_packet_table_size = byte_size_table_entry * packet_table_header->mNumberPackets;
        return 0;
    }
private:
    std::ofstream file_;
};

using AlacFileEncoderHandle = UniqueHandle<void*, AlacFileEncoderTraits>;

class AlacFileEncoder::AlacFileEncoderImpl {
public:
    void Start(const AnyMap& config) {
        const auto input_file_path = config.AsPath(FileEncoderConfig::kInputFilePath);
        const auto output_file_path = config.AsPath(FileEncoderConfig::kOutputFilePath);

        input_file_ = StreamFactory::MakeFileStream(DsdModes::DSD_MODE_PCM, input_file_path);
        output_file_ = MakeAlign<CAFFFileWriter>(output_file_path);
        input_file_->OpenFile(input_file_path);

        auto format = input_file_->GetFormat();

        input_buffer_.resize(kALACDefaultFramesPerPacket);
        output_buffer_.resize(kALACDefaultFramesPerPacket);

        handle_.reset(ALAC_LIB.InitializeEncoder(format.GetSampleRate(),
            format.GetChannels(),
            format.GetBitsPerSample(),
            kALACDefaultFramesPerPacket,
            false));
    }

    static void SetOutputFormat(const AudioFormatDescription& input_format, AudioFormatDescription *output_format) {
        if (input_format.mFormatID == kALACFormatLinearPCM) {
            output_format->mFormatID = kALACFormatAppleLossless;
            output_format->mSampleRate = input_format.mSampleRate;

            switch (input_format.mBitsPerChannel) {
            case 16:
                output_format->mFormatFlags = kTestFormatFlag_16BitSourceData;
                break;
            case 20:
                output_format->mFormatFlags = kTestFormatFlag_20BitSourceData;
                break;
            case 24:
                output_format->mFormatFlags = kTestFormatFlag_24BitSourceData;
                break;
            case 32:
                output_format->mFormatFlags = kTestFormatFlag_32BitSourceData;
                break;
            }
            output_format->mFramesPerPacket = kALACDefaultFramesPerPacket;
            output_format->mChannelsPerFrame = input_format.mChannelsPerFrame;
            output_format->mBytesPerPacket = output_format->mBytesPerFrame = output_format->mBitsPerChannel = output_format->mReserved = 0;
        }
    }

    void WriteFileChunk() {
        output_file_->WriteCAFFcaffChunk();
        AudioFormatDescription output_format;
        output_file_->WriteCAFFdescChunk(output_format);

        const auto magic_cookie_size = ALAC_LIB.GetMagicCookieSize(handle_.get());
        std::vector<uint8_t> magic_cookie(magic_cookie_size);
        ALAC_LIB.GetMagicCookie(handle_.get(), magic_cookie.data());
        output_file_->WriteCAFFkukiChunk(magic_cookie.data(), magic_cookie_size);

        int32_t input_data_size = 0;
        int32_t packet_table_size = 0;
        int32_t packet_table_bytes_left = 0;
        int32_t packet_table_size_pos = 0;
        int32_t packet_table_pos = 0;

        AudioFormatDescription input_format;
        CAFPacketTableHeader packet_table_header;
        output_file_->BuildBasePacketTable(input_format, input_data_size, &packet_table_size, &packet_table_header);
        packet_table_bytes_left = packet_table_size;

        std::vector<uint8_t> packet_table_entries(packet_table_size);
        packet_table_size += kMinCAFFPacketTableHeaderSize;
        output_file_->WriteCAFFpaktChunkHeader(&packet_table_header, packet_table_size);

        packet_table_size_pos = packet_table_pos = output_file_->GetPosition();
        packet_table_size_pos -= (sizeof(int64_t) + kMinCAFFPacketTableHeaderSize);

        packet_table_size -= kMinCAFFPacketTableHeaderSize;
        output_file_->WritePacketTableEntries(packet_table_entries);

        auto data_size_pos = output_file_->GetPosition() + sizeof(uint32_t);
        output_file_->WriteCAFFdataChunk();
        auto data_pos = output_file_->GetPosition();

        int32_t input_data_bytes_remaining = input_data_size;
        int32_t input_packet_bytes = input_format.mChannelsPerFrame * (input_format.mBitsPerChannel >> 3) * input_format.mFramesPerPacket;

        while (input_packet_bytes <= input_data_bytes_remaining) {
	        
        }
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
            if (ALAC_LIB.Encode(handle_.get(), reinterpret_cast<unsigned char*>(convert_buffer_.data()), output_buffer_.data(),
                &ret) != 0) {
                throw LibALACException(ret);
            }

            if (ret > 0) {
                /*if (!output_file_.write(reinterpret_cast<const char*>(output_buffer_.data()), ret)) {
                    break;
                }*/
            }
        }
    }
private:        
    Buffer<float> input_buffer_;
    Buffer<int16_t> convert_buffer_;
    Buffer<uint8_t> output_buffer_;
    AlignPtr<FileStream> input_file_;
    AlignPtr<CAFFFileWriter> output_file_;
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
