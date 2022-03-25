#include <fstream>
#include <base/buffer.h>
#include <base/str_utilts.h>
#include <base/exception.h>
#include <stream/bassfilestream.h>
#include <stream/basslib.h>
#include <stream/api.h>
#include <stream/basswavfileencoder.h>

namespace xamp::stream {

class BassWavFileEncoder::BassWavFileEncoderImpl {
public:
    void Start(std::wstring const& input_file_path, std::wstring const& output_file_path, std::wstring const& command) {
        DWORD flags = BASS_ENCODE_AUTOFREE;

        if (TestDsdFileFormatStd(input_file_path)) {
            stream_.SetDSDMode(DsdModes::DSD_MODE_DSD2PCM);
        }
        stream_.OpenFile(input_file_path);

        switch (stream_.GetBitDepth()) {
        case 24:
            flags |= BASS_ENCODE_FP_24BIT;
            break;
        default:
            flags |= BASS_ENCODE_FP_16BIT;
            break;
        }

        encoder_.reset(BASS.EncLib->BASS_Encode_StartACMFile(stream_.GetHStream(),
            (void*)(command.c_str()),
            flags | BASS_UNICODE,
            output_file_path.c_str()));

        if (!encoder_) {
            throw BassException();
        }
    }

    void Encode(std::function<bool(uint32_t) > const& progress) {
        constexpr uint32_t kReadSampleSize = 8192 * 4;

        auto buffer = MakeBuffer<float>(kReadSampleSize * kMaxChannel);

        uint32_t num_samples = 0;
        const auto max_duration = static_cast<uint64_t>(stream_.GetDuration());

        while (stream_.IsActive()) {
            const auto read_size = stream_.GetSamples(buffer.data(), kReadSampleSize)
                / kMaxChannel;
            if (read_size == kBassError || read_size == 0) {
                break;
            }
            num_samples += read_size;
            const auto percent = static_cast<uint32_t>(num_samples / stream_.GetFormat().GetSampleRate() * 100 / max_duration);
            if (!progress(percent)) {
                break;
            }
        }
    }

    BassFileStream stream_;
    BassStreamHandle encoder_;
};

BassWavFileEncoder::BassWavFileEncoder()
	: impl_(MakeAlign<BassWavFileEncoderImpl>()) {
}

XAMP_PIMPL_IMPL(BassWavFileEncoder)

void BassWavFileEncoder::Start(std::wstring const& input_file_path, std::wstring const& output_file_path, std::wstring const& command) {
    impl_->Start(input_file_path, output_file_path, command);
}

void BassWavFileEncoder::Encode(std::function<bool(uint32_t)> const& progress) {
    impl_->Encode(progress);
}

}
