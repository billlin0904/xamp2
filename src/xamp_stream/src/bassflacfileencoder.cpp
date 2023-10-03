#include <stream/bassflacfileencoder.h>

#include <stream/bassfilestream.h>
#include <stream/basslib.h>
#include <stream/api.h>
#include <stream/bass_utiltis.h>

#include <base/buffer.h>

XAMP_STREAM_NAMESPACE_BEGIN

class BassFlacFileEncoder::BassFlacFileEncoderImpl {
public:
    void Start(const AnyMap& config) {
        const auto input_file_path = config.AsPath(FileEncoderConfig::kInputFilePath);
        const auto output_file_path = config.AsPath(FileEncoderConfig::kOutputFilePath);
        const auto command = config.Get<std::wstring>(FileEncoderConfig::kCommand);

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

#ifdef XAMP_OS_MAC
        auto utf8_command = String::ToString(command);
        auto utf8_ouput_file_name = String::ToString(output_file_path.wstring());
        encoder_.reset(BASS.FLACEncLib->BASS_Encode_FLAC_StartFile(stream_.GetHStream(),
                                                      utf8_command.c_str(),
                                                      flags,
                                                      utf8_ouput_file_name.c_str()));
#else
        encoder_.reset(BASS.FLACEncLib->BASS_Encode_FLAC_StartFile(stream_.GetHStream(),
            command.c_str(),
            flags | BASS_UNICODE,
            output_file_path.c_str()));
#endif


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

BassFlacFileEncoder::BassFlacFileEncoder()
	: impl_(MakeAlign<BassFlacFileEncoderImpl>()) {
}

XAMP_PIMPL_IMPL(BassFlacFileEncoder)

void BassFlacFileEncoder::Start(const AnyMap& config) {
    impl_->Start(config);
}

void BassFlacFileEncoder::Encode(std::function<bool(uint32_t)> const& progress) {
    impl_->Encode(progress);
}

XAMP_STREAM_NAMESPACE_END
