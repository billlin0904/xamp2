#include <fstream>
#include <base/assert.h>
#include <base/stl.h>
#include <base/buffer.h>
#include <base/str_utilts.h>
#include <base/exception.h>
#include <stream/bassfilestream.h>
#include <stream/basslib.h>
#include <stream/api.h>
#include <stream/bass_utiltis.h>

#ifdef XAMP_OS_MAC
#include <AudioToolbox/AudioToolbox.h>
#endif

#include <stream/bassaacfileencoder.h>

namespace xamp::stream {

#ifdef XAMP_OS_WIN
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

        encoder_.reset(BASS.AACEncLib->BASS_Encode_AAC_StartFile(stream_.GetHStream(),
            command.c_str(),
            flags | BASS_UNICODE,
            output_file_path.c_str()));

        if (!encoder_) {
            throw BassException();
        }
    }

    void Encode(std::function<bool(uint32_t) > const& progress) {
        BassUtiltis::Encode(stream_, progress);
    }

    void SetEncodingProfile(const EncodingProfile& profile) {

    }

    Vector<EncodingProfile> GetAvailableEncodingProfile() {
        return {};
    }

    BassFileStream stream_;
    BassStreamHandle encoder_;
};
#else

struct XAMP_STREAM_API AudioConverterDeleter final {
    static AudioConverterRef invalid() noexcept {
        return nullptr;
    }

    static void close(AudioConverterRef value)  {
        AudioConverterDispose(value);
    }
};

using AudioConverterHandle = UniqueHandle<AudioConverterRef, AudioConverterDeleter>;

class BassAACFileEncoder::BassAACFileEncoderImpl {
public:
    BassAACFileEncoderImpl() = default;

    void Start(Path const& input_file_path, Path const& output_file_path, std::wstring const& /*command*/) {
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

        auto utf8_ouput_file_name = String::ToString(output_file_path.wstring());
        encoder_.reset(BASS.CAEncLib->BASS_Encode_StartCAFile(stream_.GetHStream(),
                                                              kAudioFileM4AType,
                                                              kAudioFormatMPEG4AAC,
                                                              flags,
                                                              256000,
                                                              utf8_ouput_file_name.c_str()));
        if (!encoder_) {
            throw BassException();
        }
    }

    void Encode(std::function<bool(uint32_t) > const& progress) {
        BassUtiltis::Encode(stream_, progress);
    }

    void SetEncodingProfile(const EncodingProfile& profile) {

    }

    Vector<EncodingProfile> GetAvailableEncodingProfile() {
        HashSet<UInt32> birates;
        GetAvailableEncodeBitRates(handle_.get(), [&](UInt32 min, UInt32 max) {
            if (!birates.contains(min)) {
                birates.insert(min);
            }

            if (!birates.contains(max)) {
                birates.insert(max);
            }
        });
        HashSet<UInt32> samplerates;
        GetAvailableEncodeSampleRates(handle_.get(), [&](const AudioValueRange &range) {
            if (!samplerates.contains((UInt32)range.mMinimum)) {
                samplerates.insert((UInt32)range.mMinimum);
            }

            if (!samplerates.contains((UInt32)range.mMaximum)) {
                samplerates.insert((UInt32)range.mMaximum);
            }
        });
        Vector<EncodingProfile> profiles;
        return profiles;
    }
private:
    void CreateAudioConverter(const AudioStreamBasicDescription &iasbd,
                              const AudioStreamBasicDescription &oasbd) {
        AudioConverterRef converter;
        AudioConverterNew(&iasbd, &oasbd, &converter);
        handle_.reset(converter);
    }

    template <typename F>
    static bool QueryConverterProperty(AudioConverterRef converter, AudioFormatPropertyID property, F &&func) {
        UInt32 size = 0;
        auto code = AudioConverterGetPropertyInfo(converter,
                                                  property,
                                                  &size,
                                                  nullptr);
        if (code || !size) {
            return false;
        }
        Vector<uint8_t> buffer(size);
        code = AudioConverterGetProperty(converter, property, &size,
                                         buffer.data());
        if (code) {
            return false;
        }
        func(size, static_cast<void *>(buffer.data()));
        return true;
    }

    template <typename F>
    static bool GetAvailableEncodeSampleRates(AudioConverterRef converter, F &&func) {
        auto helper = [&](UInt32 size, void *data) {
            auto range = static_cast<AudioValueRange *>(data);
            size_t num_ranges = size / sizeof(AudioValueRange);
            for (size_t i = 0; i < num_ranges; i++)
                func(range[i]);
        };

        return QueryConverterProperty(converter,
                                      kAudioFormatProperty_AvailableEncodeSampleRates,
                                      helper);
    }

    template <typename F>
    static bool GetAvailableEncodeBitRates(AudioConverterRef converter, F &&func) {
        auto helper = [&](UInt32 size, void *data) {
            auto range = static_cast<AudioValueRange *>(data);
            size_t num_ranges = size / sizeof(AudioValueRange);
            for (size_t i = 0; i < num_ranges; i++)
                func(static_cast<UInt32>(range[i].mMinimum),
                     static_cast<UInt32>(range[i].mMaximum));
        };
        return QueryConverterProperty(converter,
                                      kAudioFormatProperty_AvailableEncodeBitRates,
                                      helper);
    }

    BassFileStream stream_;
    BassStreamHandle encoder_;
    AudioConverterHandle handle_;
};

#endif

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

void BassAACFileEncoder::SetEncodingProfile(const EncodingProfile& profile) {
    impl_->SetEncodingProfile(profile);
}

Vector<EncodingProfile> BassAACFileEncoder::GetAvailableEncodingProfile() {
    return impl_->GetAvailableEncodingProfile();
}

}

