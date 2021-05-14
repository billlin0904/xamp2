 #include <stream/basslib.h>
#include <base/singleton.h>
#include <stream/bassexception.h>
#include <stream/bassfilestream.h>

namespace xamp::stream {

static uint32_t GetDOPSampleRate(uint32_t dsd_speed) {
    switch (dsd_speed) {
        // 64x CD
    case 64:
        return 176400;
        // 128x CD
    case 128:
        return 352800;
        // 256x CD
    case 256:
        return 705600;
    default:
        throw NotSupportFormatException();
    }
}

class BassFileStream::BassFileStreamImpl {
public:
    BassFileStreamImpl() noexcept
        : mode_(DsdModes::DSD_MODE_PCM) {
        Close();
    }

    void LoadFromFile(std::wstring const & file_path) {
        DWORD flags = 0;

        switch (mode_) {
        case DsdModes::DSD_MODE_PCM:
        case DsdModes::DSD_MODE_DSD2PCM:
            flags = BASS_SAMPLE_FLOAT;
            break;
        case DsdModes::DSD_MODE_DOP:
            // DSD-over-PCM data is 24-bit, so the BASS_SAMPLE_FLOAT flag is required.
            flags = BASS_DSD_DOP | BASS_SAMPLE_FLOAT;
            break;
        case DsdModes::DSD_MODE_NATIVE:
            flags = BASS_DSD_RAW;
            break;
        default:
            XAMP_NO_DEFAULT;
        }

        file_.Close();

        const auto use_filemap = file_path.find(L"https") == std::string::npos
    	|| file_path.find(L"http") == std::string::npos;
    	if (use_filemap) {            
            file_.Open(file_path);
    	}        

        if (mode_ == DsdModes::DSD_MODE_PCM) {
            if (use_filemap) {
                stream_.reset(BASS.BASS_StreamCreateFile(TRUE,
                    file_.GetData(),
                    0,
                    file_.GetLength(),
                    flags | BASS_STREAM_DECODE));
            } else {
                auto url = const_cast<wchar_t*>(file_path.c_str());
                stream_.reset(BASS.BASS_StreamCreateURL(
                    url,
                    0,   
                    flags | BASS_STREAM_DECODE | BASS_UNICODE | BASS_STREAM_STATUS /*| BASS_STREAM_BLOCK*/,
                    &BassFileStreamImpl::DownloadProc,
                    nullptr));
            }
            mode_ = DsdModes::DSD_MODE_PCM;
        } else {
            stream_.reset(BASS.DSDLib->BASS_DSD_StreamCreateFile(TRUE,
                file_.GetData(),
                0,
                file_.GetLength(),
                flags | BASS_STREAM_DECODE,
                0));
            if (!PrefetchFile(file_)) {
                XAMP_LOG_DEBUG("PrefetchFile return failure!");
            }
        	// BassLib DSD module default use 6dB gain.
            // 不設定的話會爆音!
            BASS.BASS_ChannelSetAttribute(stream_.get(), BASS_ATTRIB_DSD_GAIN, 0.0);
        }

        if (!stream_) {
            throw BassException();
        }

        XAMP_LOG_DEBUG("Stream running in {}", EnumToString(mode_));

        info_ = BASS_CHANNELINFO{};
        BassIfFailedThrow(BASS.BASS_ChannelGetInfo(stream_.get(), &info_));

        if (GetFormat().GetChannels() == kMaxChannel) {
            return;
        }

        if (mode_ == DsdModes::DSD_MODE_PCM) {
            mix_stream_.reset(BASS.MixLib->BASS_Mixer_StreamCreate(GetFormat().GetSampleRate(),
                kMaxChannel,
                BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | BASS_MIXER_END));
            if (!mix_stream_) {
                throw BassException();
            }
            BassIfFailedThrow(BASS.MixLib->BASS_Mixer_StreamAddChannel(mix_stream_.get(),
                stream_.get(),
                BASS_MIXER_BUFFER));
            XAMP_LOG_DEBUG("Mix stream {} channel to 2 channel", info_.chans);
            info_.chans = kMaxChannel;
        }
        else {
            throw NotSupportFormatException();
        }
    }

	static void DownloadProc(const void* buffer, DWORD length, void* user) {
    	if (length == 0) {
            auto status = static_cast<char const*>(buffer);
            XAMP_LOG_DEBUG("{}", status);
    	}        
    }

    void Close() noexcept {
        stream_.reset();
        mix_stream_.reset();
        mode_ = DsdModes::DSD_MODE_PCM;
        info_ = BASS_CHANNELINFO{};
    }

    [[nodiscard]] bool IsDsdFile() const noexcept {
        assert(stream_.is_valid());
        return info_.ctype == BASS_CTYPE_STREAM_DSD;
    }

    uint32_t GetSamples(void *buffer, uint32_t length) const noexcept {
        return static_cast<uint32_t>(InternalGetSamples(buffer, length * GetSampleSize()) / GetSampleSize());
    }

    [[nodiscard]] double GetDuration() const {
        assert(stream_.is_valid());
        const auto len = BASS.BASS_ChannelGetLength(stream_.get(), BASS_POS_BYTE);
        return BASS.BASS_ChannelBytes2Seconds(stream_.get(), len);
    }

    [[nodiscard]] AudioFormat GetFormat() const noexcept {
        assert(stream_.is_valid());
        if (mode_ == DsdModes::DSD_MODE_NATIVE) {
            return AudioFormat(DataFormat::FORMAT_DSD,
                               static_cast<uint16_t>(info_.chans),
                               ByteFormat::SINT8,
                               GetDsdSampleRate());
        } else if (mode_ == DsdModes::DSD_MODE_DOP) {
            return AudioFormat(DataFormat::FORMAT_PCM,
                               static_cast<uint16_t>(info_.chans),
                               ByteFormat::FLOAT32,
                               GetDOPSampleRate(GetDsdSpeed()));
        }
        return AudioFormat(DataFormat::FORMAT_PCM,
                           static_cast<uint16_t>(info_.chans),
                           ByteFormat::FLOAT32,
                           info_.freq);
    }

    void Seek(double stream_time) const {
        const auto pos_bytes = BASS.BASS_ChannelSeconds2Bytes(GetHStream(), stream_time);
        BassIfFailedThrow(BASS.BASS_ChannelSetPosition(GetHStream(), pos_bytes, BASS_POS_BYTE));
    }

    [[nodiscard]] uint32_t GetDsdSampleRate() const {
        float rate = 0;
        BassIfFailedThrow(BASS.BASS_ChannelGetAttribute(GetHStream(), BASS_ATTRIB_DSD_RATE, &rate));
        return static_cast<uint32_t>(rate);
    }

    [[nodiscard]] bool SupportDOP() const noexcept {
        return true;
    }

    [[nodiscard]] bool SupportDOP_AA() const noexcept {
        return false;
    }

    [[nodiscard]] bool SupportRAW() const noexcept {
        return true;
    }

    void SetDSDMode(DsdModes mode) noexcept {
        mode_ = mode;
    }

    [[nodiscard]] DsdModes GetDSDMode() const noexcept {
        return mode_;       
    }

    [[nodiscard]] uint8_t GetSampleSize() const noexcept {
        return mode_ == DsdModes::DSD_MODE_NATIVE ? sizeof(int8_t) : sizeof(float);
    }

    [[nodiscard]] DsdFormat GetDsdFormat() const noexcept {
        return DsdFormat::DSD_INT8MSB;
    }

    void SetDsdToPcmSampleRate(uint32_t sample_rate) {
        BASS.BASS_SetConfig(BASS_CONFIG_DSD_FREQ, sample_rate);
    }

    [[nodiscard]] uint32_t GetDsdSpeed() const noexcept {
        return GetDsdSampleRate() / kPcmSampleRate441;
    }
private:
    XAMP_ALWAYS_INLINE HSTREAM GetHStream() const {
        if (mix_stream_.is_valid()) {
            return mix_stream_.get();
        }
        return stream_.get();
    }

    XAMP_ALWAYS_INLINE uint32_t InternalGetSamples(void* buffer, uint32_t length) const noexcept {
        const auto bytes_read = BASS.BASS_ChannelGetData(GetHStream(), buffer, length);
        if (bytes_read == kBassError) {
            return 0;
        }
        return static_cast<uint32_t>(bytes_read);
    }

    DsdModes mode_;
    BassStreamHandle stream_;
    BassStreamHandle mix_stream_;
    BASS_CHANNELINFO info_;
    MemoryMappedFile file_;
};

BassFileStream::BassFileStream()
    : stream_(MakeAlign<BassFileStreamImpl>()) {
}

XAMP_PIMPL_IMPL(BassFileStream)

void BassFileStream::OpenFile(std::wstring const & file_path)  {
    stream_->LoadFromFile(file_path);
}

void BassFileStream::Close() noexcept {
    stream_->Close();
}

double BassFileStream::GetDuration() const {
    return stream_->GetDuration();
}

AudioFormat BassFileStream::GetFormat() const noexcept {
    return stream_->GetFormat();
}

void BassFileStream::Seek(double stream_time) const {
    stream_->Seek(stream_time);
}

uint32_t BassFileStream::GetSamples(void *buffer, uint32_t length) const noexcept {
    return stream_->GetSamples(buffer, length);
}

std::string_view BassFileStream::GetDescription() const noexcept {
    return "BASS";
}

void BassFileStream::SetDSDMode(DsdModes mode) noexcept {
    stream_->SetDSDMode(mode);
}

DsdModes BassFileStream::GetDsdMode() const noexcept {
    return stream_->GetDSDMode();
}

bool BassFileStream::IsDsdFile() const noexcept {
    return stream_->IsDsdFile();
}

uint32_t BassFileStream::GetDsdSampleRate() const {
    return stream_->GetDsdSampleRate();
}

uint8_t BassFileStream::GetSampleSize() const noexcept {
    return stream_->GetSampleSize();
}

DsdFormat BassFileStream::GetDsdFormat() const noexcept {
    return stream_->GetDsdFormat();
}

void BassFileStream::SetDsdToPcmSampleRate(uint32_t sample_rate) {
    stream_->SetDsdToPcmSampleRate(sample_rate);
}

uint32_t BassFileStream::GetDsdSpeed() const noexcept {
    return stream_->GetDsdSpeed();
}

std::set<std::string> BassFileStream::GetSupportFileExtensions() {
    return Singleton<BassLib>::GetInstance().GetSupportFileExtensions();
}

void LoadBassLib() {
    if (!BASS.IsLoaded()) {
        (void)Singleton<BassLib>::GetInstance().Load();
    }
    BASS.MixLib = MakeAlign<BassMixLib>();
    XAMP_LOG_DEBUG("Load BassMixLib {} successfully.", GetBassVersion(BASS.MixLib->BASS_Mixer_GetVersion()));
    BASS.DSDLib = MakeAlign<BassDSDLib>();
    XAMP_LOG_DEBUG("Load BassDSDLib successfully.");
    BASS.FxLib = MakeAlign<BassFxLib>();
    XAMP_LOG_DEBUG("Load BassFxLib successfully.");
    BASS.BASS_SetConfig(BASS_CONFIG_DSD_FREQ, 88200);
    XAMP_LOG_DEBUG("Load BassDSDLib successfully.");    
}

void FreeBassLib() {
    Singleton<BassLib>::GetInstance().Free();
}

}
