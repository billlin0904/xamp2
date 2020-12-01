#include <stream/basslib.h>
#include <base/singleton.h>
#include <stream/bassexception.h>
#include <stream/bassfilestream.h>

namespace xamp::stream {

static void EnsureDsdDecoderInit() {
    if (!Singleton<BassLib>::GetInstance().DSDLib) {
        Singleton<BassLib>::GetInstance().DSDLib = MakeAlign<BassDSDLib>();
        // TODO: Set hightest dsd2pcm samplerate?
        Singleton<BassLib>::GetInstance().BASS_SetConfig(BASS_CONFIG_DSD_FREQ,  174000);
    }

}

static bool TestDsdFileFormat(std::wstring const & file_path) {
    BassStreamHandle stream;

    EnsureDsdDecoderInit();

    stream.reset(Singleton<BassLib>::GetInstance().DSDLib->BASS_DSD_StreamCreateFile(FALSE,
        file_path.data(),
        0,
        0,
        BASS_DSD_RAW | BASS_STREAM_DECODE | BASS_UNICODE,
        0));

    return stream.is_valid();
}

class BassFileStream::BassFileStreamImpl {
public:
    BassFileStreamImpl() noexcept
        : enable_file_mapped_(true)
        , mode_(DsdModes::DSD_MODE_PCM) {
        info_ = BASS_CHANNELINFO{};
    }

    void LoadFromFile(std::wstring const & file_path) {
        info_ = BASS_CHANNELINFO{};

        DWORD flags = 0;

        switch (mode_) {
        case DsdModes::DSD_MODE_PCM:
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

        if (!TestDsdFileFormat(file_path)) {
            if (enable_file_mapped_) {
                file_.Open(file_path);
                stream_.reset(Singleton<BassLib>::GetInstance().BASS_StreamCreateFile(TRUE,
                                                                        file_.GetData(),
                                                                        0,
                                                                        file_.GetLength(),
                                                                        flags | BASS_STREAM_DECODE));
            }
            else {
                stream_.reset(Singleton<BassLib>::GetInstance().BASS_StreamCreateFile(FALSE,
                                                                        file_path.data(),
                                                                        0,
                                                                        0,
                                                                        flags | BASS_STREAM_DECODE | BASS_UNICODE));
            }

            // BassLib DSD module default use 6dB gain.
            // 不設定的話會爆音!
            Singleton<BassLib>::GetInstance().BASS_ChannelSetAttribute(stream_.get(), BASS_ATTRIB_DSD_GAIN, 0.0);

            mode_ = DsdModes::DSD_MODE_PCM;
        } else {
            EnsureDsdDecoderInit();

            if (enable_file_mapped_) {
                file_.Open(file_path);
                stream_.reset(Singleton<BassLib>::GetInstance().DSDLib->BASS_DSD_StreamCreateFile(TRUE,
                                                                                    file_.GetData(),
                                                                                    0,
                                                                                    file_.GetLength(),
                                                                                    flags | BASS_STREAM_DECODE,
                                                                                    0));
                PrefactchFile(file_);
            }
            else {
                stream_.reset(Singleton<BassLib>::GetInstance().DSDLib->BASS_DSD_StreamCreateFile(FALSE,
                                                                                    file_path.data(),
                                                                                    0,
                                                                                    0,
                                                                                    flags | BASS_STREAM_DECODE | BASS_UNICODE,
                                                                                    0));
            }            
        }

        XAMP_LOG_DEBUG("Stream running in {}", EnumToString(mode_));

        if (!stream_) {
            throw BassException(Singleton<BassLib>::GetInstance().BASS_ErrorGetCode());
        }

        info_ = BASS_CHANNELINFO{};	
        BassIfFailedThrow(Singleton<BassLib>::GetInstance().BASS_ChannelGetInfo(stream_.get(), &info_));

        if (GetFormat().GetChannels() != kMaxChannel) {
            throw NotSupportFormatException();
        }
    }

    void Close() noexcept {
        stream_.reset();
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

    double GetDuration() const {
        assert(stream_.is_valid());
        const auto len = Singleton<BassLib>::GetInstance().BASS_ChannelGetLength(stream_.get(), BASS_POS_BYTE);
        return Singleton<BassLib>::GetInstance().BASS_ChannelBytes2Seconds(stream_.get(), len);
    }

    AudioFormat GetFormat() const noexcept {
        assert(stream_.is_valid());
        if (mode_ == DsdModes::DSD_MODE_NATIVE) {
            return AudioFormat(DataFormat::FORMAT_DSD,
                               info_.chans,
                               ByteFormat::SINT8,
                               GetDsdSampleRate());
        } else if (mode_ == DsdModes::DSD_MODE_DOP) {
            return AudioFormat(DataFormat::FORMAT_PCM,
                               info_.chans,
                               ByteFormat::FLOAT32,
                               GetDOPSampleRate(GetDsdSpeed()));
        }
        return AudioFormat(DataFormat::FORMAT_PCM,
                           info_.chans,
                           ByteFormat::FLOAT32,
                           info_.freq);
    }

    void Seek(double stream_time) const {
        assert(stream_.is_valid());
        const auto pos_bytes = Singleton<BassLib>::GetInstance().BASS_ChannelSeconds2Bytes(stream_.get(), stream_time);
        BassIfFailedThrow(Singleton<BassLib>::GetInstance().BASS_ChannelSetPosition(stream_.get(), pos_bytes, BASS_POS_BYTE));
    }

    uint32_t GetDsdSampleRate() const {
        float rate = 0;
        BassIfFailedThrow(Singleton<BassLib>::GetInstance().BASS_ChannelGetAttribute(stream_.get(), BASS_ATTRIB_DSD_RATE, &rate));
        return static_cast<uint32_t>(rate);
    }

    bool SupportDOP() const noexcept {
        return true;
    }

    bool SupportDOP_AA() const noexcept {
        return false;
    }

    bool SupportRAW() const noexcept {
        return true;
    }

    void SetDSDMode(DsdModes mode) noexcept {
        mode_ = mode;
    }

    DsdModes GetDSDMode() const noexcept {
        return mode_;       
    }

    uint8_t GetSampleSize() const noexcept {
        return mode_ == DsdModes::DSD_MODE_NATIVE ? sizeof(int8_t) : sizeof(float);
    }

    DsdFormat GetDsdFormat() const noexcept {
        return DsdFormat::DSD_INT8MSB;
    }

    void SetDsdToPcmSampleRate(uint32_t samplerate) {
        Singleton<BassLib>::GetInstance().BASS_SetConfig(BASS_CONFIG_DSD_FREQ, samplerate);
    }

    uint32_t GetDsdSpeed() const noexcept {
        return GetDsdSampleRate() / kPcmSampleRate441;
    }

    uint64_t GetTotalFrames() const {
        return 0;
    }
private:
    XAMP_ALWAYS_INLINE uint32_t InternalGetSamples(void *buffer, uint32_t length) const noexcept {
        const auto bytes_read = Singleton<BassLib>::GetInstance().BASS_ChannelGetData(stream_.get(), buffer, length);
        if (bytes_read == kBassError) {
            return 0;
        }
        return static_cast<uint32_t>(bytes_read);
    }

    bool enable_file_mapped_;
    DsdModes mode_;
    BassStreamHandle stream_;
    BASS_CHANNELINFO info_;
    MemoryMappedFile file_;
};

BassFileStream::BassFileStream()
    : stream_(MakeAlign<BassFileStreamImpl>()) {
}

XAMP_PIMPL_IMPL(BassFileStream)

void BassFileStream::LoadBassLib() {
    if (!Singleton<BassLib>::GetInstance().IsLoaded()) {
        Singleton<BassLib>::GetInstance().Load();
    }
}

void BassFileStream::FreeBassLib() {
    Singleton<BassLib>::GetInstance().Free();
}

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

void BassFileStream::SetDsdToPcmSampleRate(uint32_t samplerate) {
    stream_->SetDsdToPcmSampleRate(samplerate);
}

uint32_t BassFileStream::GetDsdSpeed() const noexcept {
    return stream_->GetDsdSpeed();
}

uint64_t BassFileStream::GetTotalFrames() const {
    return stream_->GetTotalFrames();
}

}
