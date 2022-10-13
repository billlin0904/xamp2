#include <fstream>
#include <base/buffer.h>
#include <base/str_utilts.h>
#include <base/exception.h>
#include <stream/bassfilestream.h>
#include <stream/basslib.h>
#include <stream/api.h>
#include <stream/bass_utiltis.h>
#include <stream/bassaacfileencoder.h>

namespace xamp::stream {

class BassAACFileEncoder::BassAACFileEncoderImpl {
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

#ifdef XAMP_OS_MAC
        auto utf8_command = String::ToString(command);
        auto utf8_ouput_file_name = String::ToString(output_file_path.wstring());
        encoder_.reset(BASS.AACEncLib->BASS_Encode_AAC_StartFile(stream_.GetHStream(),
            utf8_command.c_str(),
            flags,
            utf8_ouput_file_name.c_str()));
#else
        encoder_.reset(BASS.AACEncLib->BASS_Encode_AAC_StartFile(stream_.GetHStream(),
            command.c_str(),
            flags | BASS_UNICODE,
            output_file_path.c_str()));
#endif

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

BassAACFileEncoder::BassAACFileEncoder()
	: impl_(MakeAlign<BassAACFileEncoderImpl>()) {
}

XAMP_PIMPL_IMPL(BassAACFileEncoder)

void BassAACFileEncoder::Start(Path const& input_file_path, Path const& output_file_path, std::wstring const& command) {
    impl_->Start(input_file_path, output_file_path, command);
}

void BassAACFileEncoder::Encode(std::function<bool(uint32_t)> const& progress) {
    impl_->Encode(progress);
}

}