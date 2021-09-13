#include <fstream>
#include <base/buffer.h>
#include <base/str_utilts.h>
#include <base/exception.h>
#include <stream/bassfilestream.h>
#include <stream/basslib.h>
#include <stream/bassfileencoder.h>

namespace xamp::stream {

class BassFileEncoder::BassFileEncoderImpl {
public:
    ~BassFileEncoderImpl() {
        Stop();
    }

    void Start(std::wstring const& input_file_path, std::wstring const& output_file_path, std::wstring const& command) {
        DWORD flags = 0;

        stream_.OpenFile(input_file_path);

        switch (stream_.GetBitDepth()) {
        case 24:
            flags = BASS_ENCODE_FP_24BIT;
            break;
        default:
            flags = BASS_ENCODE_FP_16BIT;
            break;
        }

#ifdef XAMP_OS_MAC
        auto utf8_command = String::ToString(command);
        auto utf8_ouput_file_name = String::ToString(output_file_path);
        encoder_.reset(BASS.EncLib->BASS_Encode_FLAC_StartFile(stream_.GetHStream(),
                                                      utf8_command.c_str(),
                                                      flags,
                                                      utf8_ouput_file_name.c_str()));
#else
        encoder_.reset(BASS.EncLib->BASS_Encode_FLAC_StartFile(stream_.GetHStream(),
                                                               input_file_path.c_str(),
                                                               flags | BASS_UNICODE,
                                                               output_file_path.c_str()));
#endif


        if (!encoder_) {
            throw BassException();
        }
    }

    void Encode(std::function<bool(uint32_t) > const& progress) {
        constexpr uint32_t kReadSampleSize = 8192 * 4;

        auto buffer = MakeBuffer<float>(kReadSampleSize * kMaxChannel);

        uint32_t num_samples = 0;
        const auto max_duration = static_cast<uint64_t>(stream_.GetDuration());

        while (true) {
            const auto read_size = stream_.GetSamples(buffer.data(), kReadSampleSize)
                / kMaxChannel;
            if (read_size == kBassError || read_size == 0) {
                break;
            }
            num_samples += read_size;
            const auto percent = static_cast<uint32_t>((num_samples / stream_.GetFormat().GetSampleRate() * 100) / max_duration);
            if (!progress(percent)) {
                break;
            }
        }
        Stop();
    }

	void Stop() {
        BASS.EncLib->BASS_Encode_Stop(encoder_.get());
	}

    BassFileStream stream_;
    BassStreamHandle encoder_;
};

BassFileEncoder::BassFileEncoder()
	: impl_(MakeAlign<BassFileEncoderImpl>()) {
}

XAMP_PIMPL_IMPL(BassFileEncoder)

void BassFileEncoder::Start(std::wstring const& input_file_path, std::wstring const& output_file_path, std::wstring const& command) {
    impl_->Start(input_file_path, output_file_path, command);
}

void BassFileEncoder::Encode(std::function<bool(uint32_t)> const& progress) {
    impl_->Encode(progress);
}

}
