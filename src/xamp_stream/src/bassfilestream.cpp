// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <bass/bass.h>
#include <bass/bassdsd.h>

#include <base/dll.h>
#include <base/stl.h>
#include <base/exception.h>
#include <base/unique_handle.h>
#include <base/memory.h>
#include <base/memory_mapped_file.h>
#include <base/logger.h>

#include <stream/bassexception.h>
#include <stream/bassfilestream.h>

namespace xamp::stream {

static constexpr DWORD kBassError{ 0xFFFFFFFF };
static constexpr int32_t kPcmSampleRate441 = { 44100 };

template <typename T>
constexpr uint8_t HiByte(T val) noexcept {
    return static_cast<uint8_t>(val >> 8);
}

template <typename T>
constexpr uint8_t LowByte(T val) noexcept {
    return static_cast<uint8_t>(val);
}

template <typename T>
constexpr uint16_t HiWord(T val) noexcept {
    return static_cast<uint16_t>((uint32_t(val) >> 16) & 0xFFFF);
}

template <typename T>
constexpr uint16_t LoWord(T val) noexcept {
    return static_cast<uint16_t>(uint32_t(val) & 0xFFFF);
}

#define BassIfFailedThrow(result) \
	do {\
		if (!(result)) {\
			throw BassException(BassLib::Instance().BASS_ErrorGetCode());\
		}\
	} while (false)

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

class BassDSDLib final {
public:
    BassDSDLib() try
#ifdef XAMP_OS_WIN
        : module_(LoadModule("bassdsd.dll"))
#else
        : module_(LoadModule("libbassdsd.dylib"))
#endif
        , BASS_DSD_StreamCreateFile(module_, "BASS_DSD_StreamCreateFile") {
    }
    catch (const Exception & e) {
        XAMP_LOG_ERROR("{}", e.GetErrorMessage());
    }

    XAMP_DISABLE_COPY(BassDSDLib)

private:
    ModuleHandle module_;

public:
    DllFunction<HSTREAM(BOOL, void const *, QWORD, QWORD, DWORD, DWORD)> BASS_DSD_StreamCreateFile;
};

class BassLib final {
public:
    static XAMP_ALWAYS_INLINE BassLib & Instance() {
        static BassLib lib;
        return lib;
    }

    ~BassLib() {
        plugins_.clear();

        if (module_.is_valid()) {
            try {
                BassLib::Instance().BASS_Free();
            }
            catch (const Exception & e) {
                XAMP_LOG_INFO("{}", e.what());
            }
        }
    }

    XAMP_ALWAYS_INLINE bool IsLoaded() const noexcept {
        return !plugins_.empty();
    }

    void Load() {
        if (IsLoaded()) {
            return;
        }

        BassLib::Instance().BASS_Init(0, 44100, 0, nullptr, nullptr);
#ifdef XAMP_OS_WIN
        // Disable Media Foundation
        BassLib::Instance().BASS_SetConfig(BASS_CONFIG_MF_DISABLE, true);
        BassLib::Instance().BASS_SetConfig(BASS_CONFIG_MF_VIDEO, false);
        LoadPlugin("bass_aac.dll");
        LoadPlugin("bassflac.dll");
        LoadPlugin("bass_ape.dll");
#else
        LoadPlugin("bass_aac.dylib");
#endif
	}

    XAMP_DISABLE_COPY(BassLib)

private:
    BassLib() try
#ifdef XAMP_OS_WIN
        : module_(LoadModule("bass.dll"))
#else
        : module_(LoadModule("libbass.dylib"))
#endif
        , BASS_Init(module_, "BASS_Init")
        , BASS_SetConfig(module_, "BASS_SetConfig")
        , BASS_PluginLoad(module_, "BASS_PluginLoad")
        , BASS_PluginGetInfo(module_, "BASS_PluginGetInfo")
        , BASS_Free(module_, "BASS_Free")
        , BASS_StreamCreateFile(module_, "BASS_StreamCreateFile")
        , BASS_ChannelGetInfo(module_, "BASS_ChannelGetInfo")
        , BASS_StreamFree(module_, "BASS_StreamFree")
        , BASS_PluginFree(module_, "BASS_PluginFree")
        , BASS_ChannelGetData(module_, "BASS_ChannelGetData")
        , BASS_ChannelGetLength(module_, "BASS_ChannelGetLength")
        , BASS_ChannelBytes2Seconds(module_, "BASS_ChannelBytes2Seconds")
        , BASS_ChannelSeconds2Bytes(module_, "BASS_ChannelSeconds2Bytes")
        , BASS_ChannelSetPosition(module_, "BASS_ChannelSetPosition")
        , BASS_ErrorGetCode(module_, "BASS_ErrorGetCode")
        , BASS_ChannelGetAttribute(module_, "BASS_ChannelGetAttribute")
        , BASS_ChannelSetAttribute(module_, "BASS_ChannelSetAttribute") {
    }
    catch (const Exception &e) {
        XAMP_LOG_ERROR("{}", e.GetErrorMessage());
    }

    ModuleHandle module_;

public:
    XAMP_DECLARE_DLL(BASS_Init) BASS_Init;
    XAMP_DECLARE_DLL(BASS_SetConfig) BASS_SetConfig;
    DllFunction<HPLUGIN(const char *, DWORD)> BASS_PluginLoad;
    XAMP_DECLARE_DLL(BASS_PluginGetInfo) BASS_PluginGetInfo;
    XAMP_DECLARE_DLL(BASS_Free) BASS_Free;
    DllFunction<HSTREAM(BOOL, const void *, QWORD, QWORD, DWORD)> BASS_StreamCreateFile;
    XAMP_DECLARE_DLL(BASS_ChannelGetInfo) BASS_ChannelGetInfo;
    XAMP_DECLARE_DLL(BASS_StreamFree) BASS_StreamFree;
    XAMP_DECLARE_DLL(BASS_PluginFree) BASS_PluginFree;
    XAMP_DECLARE_DLL(BASS_ChannelGetData) BASS_ChannelGetData;
    XAMP_DECLARE_DLL(BASS_ChannelGetLength) BASS_ChannelGetLength;
    XAMP_DECLARE_DLL(BASS_ChannelBytes2Seconds) BASS_ChannelBytes2Seconds;
    XAMP_DECLARE_DLL(BASS_ChannelSeconds2Bytes) BASS_ChannelSeconds2Bytes;
    XAMP_DECLARE_DLL(BASS_ChannelSetPosition) BASS_ChannelSetPosition;
    XAMP_DECLARE_DLL(BASS_ErrorGetCode) BASS_ErrorGetCode;
    XAMP_DECLARE_DLL(BASS_ChannelGetAttribute) BASS_ChannelGetAttribute;
    XAMP_DECLARE_DLL(BASS_ChannelSetAttribute) BASS_ChannelSetAttribute;

    AlignPtr<BassDSDLib> DSDLib;

private:
    void LoadPlugin(std::string const & file_name) {
        BassPluginHandle plugin(BassLib::Instance().BASS_PluginLoad(file_name.c_str(), 0));
        if (!plugin) {
            XAMP_LOG_DEBUG("Load {} failure.", file_name);
            return;
        }

        const auto info = BassLib::Instance().BASS_PluginGetInfo(plugin.get());
        const int major_version(HiByte(HiWord(info->version)));
        const int minor_version(LowByte(HiWord(info->version)));
        const int micro_version(HiByte(LoWord(info->version)));
        const int build_version(LowByte(LoWord(info->version)));

        std::ostringstream ostr;
        ostr << major_version << "."
             << minor_version << "."
             << micro_version << "."
             << build_version;
        XAMP_LOG_DEBUG("Load {} {} successfully.", file_name, ostr.str());

        plugins_[file_name] = std::move(plugin);
    }

    struct BassPluginLoadTraits final {
        static HPLUGIN invalid() noexcept {
            return 0;
        }

        static void close(HPLUGIN value) noexcept {
            BassLib::Instance().BASS_PluginFree(value);
        }
    };

    using BassPluginHandle = UniqueHandle<HPLUGIN, BassPluginLoadTraits>;

    RobinHoodHashMap<std::string, BassPluginHandle> plugins_;
};

struct BassStreamTraits final {
    static HSTREAM invalid() noexcept {
        return 0;
    }

    static void close(HSTREAM value) noexcept {
        BassLib::Instance().BASS_StreamFree(value);
    }
};

using BassStreamHandle = UniqueHandle<HSTREAM, BassStreamTraits>;

static void EnsureDsdDecoderInit() {
    if (!BassLib::Instance().DSDLib) {
        BassLib::Instance().DSDLib = MakeAlign<BassDSDLib>();
        // TODO: Set hightest dsd2pcm samplerate?
        BassLib::Instance().BASS_SetConfig(BASS_CONFIG_DSD_FREQ, 88200);
    }

}

static bool TestDsdFileFormat(std::wstring const & file_path) {
    BassStreamHandle stream;

    EnsureDsdDecoderInit();

    MemoryMappedFile file;
    file.Open(file_path);
    stream.reset(BassLib::Instance().DSDLib->BASS_DSD_StreamCreateFile(TRUE,
                                                                       file.GetData(),
                                                                       0,
                                                                       file.GetLength(),
                                                                       BASS_DSD_RAW | BASS_STREAM_DECODE,
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

        if (mode_ == DsdModes::DSD_MODE_PCM && !TestDsdFileFormat(file_path)) {
            if (enable_file_mapped_) {
                file_.Open(file_path);
                stream_.reset(BassLib::Instance().BASS_StreamCreateFile(TRUE,
                                                                        file_.GetData(),
                                                                        0,
                                                                        file_.GetLength(),
                                                                        flags | BASS_STREAM_DECODE));
            }
            else {
                stream_.reset(BassLib::Instance().BASS_StreamCreateFile(FALSE,
                                                                        file_path.data(),
                                                                        0,
                                                                        0,
                                                                        flags | BASS_STREAM_DECODE | BASS_UNICODE));
            }

            // BassLib DSD module default use 6dB gain.
            // 不設定的話會爆音!
            BassLib::Instance().BASS_ChannelSetAttribute(stream_.get(), BASS_ATTRIB_DSD_GAIN, 0.0);
        } else {
            EnsureDsdDecoderInit();

            if (enable_file_mapped_) {
                file_.Open(file_path);
                stream_.reset(BassLib::Instance().DSDLib->BASS_DSD_StreamCreateFile(TRUE,
                                                                                    file_.GetData(),
                                                                                    0,
                                                                                    file_.GetLength(),
                                                                                    flags | BASS_STREAM_DECODE,
                                                                                    0));
                PrefactchFile(file_);
            }
            else {
                stream_.reset(BassLib::Instance().DSDLib->BASS_DSD_StreamCreateFile(FALSE,
                                                                                    file_path.data(),
                                                                                    0,
                                                                                    0,
                                                                                    flags | BASS_STREAM_DECODE | BASS_UNICODE,
                                                                                    0));
            }            
        }

        XAMP_LOG_DEBUG("Stream running in {}", EnumToString(mode_));

        if (!stream_) {
            throw BassException(BassLib::Instance().BASS_ErrorGetCode());
        }

        info_ = BASS_CHANNELINFO{};	
        BassIfFailedThrow(BassLib::Instance().BASS_ChannelGetInfo(stream_.get(), &info_));

        if (GetFormat().GetChannels() != kMaxChannel) {
            throw NotSupportFormatException();
        }
    }

    void Close() {
        stream_.reset();
        mode_ = DsdModes::DSD_MODE_PCM;
        info_ = BASS_CHANNELINFO{};
    }

    bool IsDsdFile() const noexcept {
        assert(stream_.is_valid());
        return info_.ctype == BASS_CTYPE_STREAM_DSD;
    }

    uint32_t GetSamples(void *buffer, uint32_t length) const noexcept {
        return uint32_t(InternalGetSamples(buffer, length * GetSampleSize()) / GetSampleSize());
    }

    double GetDuration() const {
        assert(stream_.is_valid());
        const auto len = BassLib::Instance().BASS_ChannelGetLength(stream_.get(), BASS_POS_BYTE);
        return BassLib::Instance().BASS_ChannelBytes2Seconds(stream_.get(), len);
    }

    AudioFormat GetFormat() const {
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
        const auto pos_bytes = BassLib::Instance().BASS_ChannelSeconds2Bytes(stream_.get(), stream_time);
        BassIfFailedThrow(BassLib::Instance().BASS_ChannelSetPosition(stream_.get(), pos_bytes, BASS_POS_BYTE));
    }

    uint32_t GetDsdSampleRate() const {
        float rate = 0;
        BassIfFailedThrow(BassLib::Instance().BASS_ChannelGetAttribute(stream_.get(), BASS_ATTRIB_DSD_RATE, &rate));
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

    uint32_t GetSampleSize() const noexcept {
        return mode_ == DsdModes::DSD_MODE_NATIVE ? sizeof(int8_t) : sizeof(float);
    }

    DsdFormat GetDsdFormat() const noexcept {
        return DsdFormat::DSD_INT8MSB;
    }

    void SetDsdToPcmSampleRate(uint32_t samplerate) {
        BassLib::Instance().BASS_SetConfig(BASS_CONFIG_DSD_FREQ, samplerate);
    }

    uint32_t GetDsdSpeed() const noexcept {
        return GetDsdSampleRate() / kPcmSampleRate441;
    }
private:
    XAMP_ALWAYS_INLINE uint32_t InternalGetSamples(void *buffer, uint32_t length) const noexcept {
        const auto bytes_read = BassLib::Instance().BASS_ChannelGetData(stream_.get(), buffer, length);
        if (bytes_read == kBassError) {
            return 0;
        }
        return uint32_t(bytes_read);
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
    if (!BassLib::Instance().IsLoaded()) {
        BassLib::Instance().Load();
    }
}

void BassFileStream::OpenFromFile(std::wstring const & file_path)  {
    stream_->LoadFromFile(file_path);
}

void BassFileStream::Close() {
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

uint32_t BassFileStream::GetSampleSize() const noexcept {
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

}
