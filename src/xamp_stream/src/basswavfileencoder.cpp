#include <fstream>
#include <base/buffer.h>
#include <base/str_utilts.h>
#include <base/exception.h>
#include <stream/bassfilestream.h>
#include <stream/basslib.h>
#include <stream/api.h>
#include <stream/bass_utiltis.h>
#include <stream/basswavfileencoder.h>

namespace xamp::stream {

class BassWavFileEncoder::BassWavFileEncoderImpl {
public:
    void Start(Path const& input_file_path, Path const& output_file_path, std::wstring const& command) {
        DWORD flags = BASS_ENCODE_AUTOFREE;

        if (TestDsdFileFormatStd(input_file_path.wstring())) {
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

        encoder_.reset(BASS.EncLib->BASS_Encode_StartACMFile(stream_.GetHStream(),
            (void*)command.c_str(),
            flags | BASS_UNICODE,
            output_file_path.wstring().c_str()));

        if (!encoder_) {
            throw BassException();
        }
    }

    void Encode(std::function<bool(uint32_t) > const& progress) {
        BassUtiltis::Encode(stream_, progress);
    }

    BassFileStream stream_;
    BassStreamHandle encoder_;
};

BassWavFileEncoder::BassWavFileEncoder()
	: impl_(MakeAlign<BassWavFileEncoderImpl>()) {
}

XAMP_PIMPL_IMPL(BassWavFileEncoder)

void BassWavFileEncoder::Start(Path const& input_file_path, Path const& output_file_path, std::wstring const& command) {
    impl_->Start(input_file_path, output_file_path, command);
}

void BassWavFileEncoder::Encode(std::function<bool(uint32_t)> const& progress) {
    impl_->Encode(progress);
}

}
