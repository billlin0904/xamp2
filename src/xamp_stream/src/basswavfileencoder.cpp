#include <fstream>
#include <base/buffer.h>
#include <base/str_utilts.h>
#include <base/exception.h>
#include <base/logger_impl.h>
#include <stream/bassfilestream.h>
#include <stream/basslib.h>
#include <stream/api.h>
#include <stream/bass_utiltis.h>
#include <stream/basswavfileencoder.h>

namespace xamp::stream {

class BassWavFileEncoder::BassWavFileEncoderImpl {
public:
    void Start(Path const& input_file_path, Path const& output_file_path, std::wstring const&) {
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

        /*const auto acm_form_len = BASS.EncLib->BASS_Encode_GetACMFormat(0, nullptr, 0, nullptr, 0);
        std::vector<uint8_t> buffer(acm_form_len);
        BASS.EncLib->BASS_Encode_GetACMFormat(stream_.GetHStream(), buffer.data(), acm_form_len, nullptr, BASS_ACM_DEFAULT);

        std::ostringstream ostr;
        for (auto ch : buffer) {
            ostr << "0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<uint32_t>(ch) << ",";
        }

        XAMP_LOG_DEBUG("Format: {}", ostr.str());*/

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
