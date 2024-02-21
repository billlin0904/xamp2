#include <stream/basswavfileencoder.h>

#include <stream/bassfilestream.h>
#include <stream/basslib.h>
#include <stream/api.h>
#include <stream/bass_utiltis.h>

#include <base/logger_impl.h>

#include <fstream>

XAMP_STREAM_NAMESPACE_BEGIN

class BassWavFileEncoder::BassWavFileEncoderImpl {
public:
    void Start(const AnyMap& config) {
        const auto input_file_path = config.AsPath(FileEncoderConfig::kInputFilePath);
        const auto output_file_path = config.AsPath(FileEncoderConfig::kOutputFilePath);

        DWORD flags = BASS_ENCODE_AUTOFREE;

        if (IsDsdFile(input_file_path.wstring())) {
            stream_.SetDSDMode(DsdModes::DSD_MODE_DSD2PCM);
        }
        stream_.OpenFile(input_file_path.wstring());

        switch (stream_.GetBitDepth()) {
        case 24:
            flags |= BASS_ENCODE_FP_24BIT;
            break;
        default:
            flags |= BASS_ENCODE_FP_16BIT;
            break;
        }

        // 2ch,44100Khz,PCM format.
        constexpr std::array<uint8_t, 50> buffer{
            0x01,0x00,0x02,0x00,0x44,0xac,0x00,0x00,0x10,0xb1,0x02,0x00,0x04,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00
        };

        encoder_.reset(BASS.EncLib->BASS_Encode_StartACMFile(stream_.GetHStream(),
            (void*)buffer.data(),
            flags | BASS_UNICODE,
            output_file_path.wstring().c_str()));

        if (!encoder_) {
            throw BassException();
        }
    }

    void Encode(std::function<bool(uint32_t) > const& progress) {
        bass_utiltis::Encode(stream_, progress);
    }

    BassFileStream stream_;
    BassStreamHandle encoder_;
};

BassWavFileEncoder::BassWavFileEncoder()
	: impl_(MakeAlign<BassWavFileEncoderImpl>()) {
}

XAMP_PIMPL_IMPL(BassWavFileEncoder)

void BassWavFileEncoder::Start(const AnyMap& config) {
    impl_->Start(config);
}

void BassWavFileEncoder::Encode(std::function<bool(uint32_t)> const& progress) {
    impl_->Encode(progress);
}

XAMP_STREAM_NAMESPACE_END
