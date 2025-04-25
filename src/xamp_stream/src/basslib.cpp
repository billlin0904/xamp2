#include <stream/basslib.h>

#include <base/singleton.h>
#include <base/logger_impl.h>
#include <base/str_utilts.h>

#include <bass/bassdsd.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(BASS);

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
    return static_cast<uint16_t>((static_cast<uint32_t>(val) >> 16) & 0xFFFF);
}

template <typename T>
constexpr uint16_t LoWord(T val) noexcept {
    return static_cast<uint16_t>(static_cast<uint32_t>(val) & 0xFFFF);
}

std::string GetBassVersion(uint32_t version) {
    const uint32_t major_version(HiByte(HiWord(version)));
    const uint32_t minor_version(LowByte(HiWord(version)));
    const uint32_t micro_version(HiByte(LoWord(version)));
    const uint32_t build_version(LowByte(LoWord(version)));

    std::ostringstream ostr;
    ostr << major_version << "."
        << minor_version << "."
        << micro_version << "."
        << build_version;

    return ostr.str();
}

BassDSDLib::BassDSDLib() try
    : module_(OpenSharedLibrary("bassdsd"))
    , XAMP_LOAD_DLL_API(BASS_DSD_StreamCreateFile) {
}
catch (const Exception& e) {
    XAMP_LOG_E(BASS_LIB.logger, "{}", e.GetErrorMessage());
}

BassMixLib::BassMixLib() try
    : module_(OpenSharedLibrary("bassmix"))
    , XAMP_LOAD_DLL_API(BASS_Mixer_StreamCreate)
    , XAMP_LOAD_DLL_API(BASS_Mixer_StreamAddChannel)
    , XAMP_LOAD_DLL_API(BASS_Mixer_GetVersion) {
}
catch (const Exception& e) {
    XAMP_LOG_E(BASS_LIB.logger, "{}", e.GetErrorMessage());
}

std::string BassMixLib::GetName() const {
    return GetSharedLibraryName("bassmix");
}

BassFxLib::BassFxLib() try
    : module_(OpenSharedLibrary("bass_fx"))
    , XAMP_LOAD_DLL_API(BASS_FX_TempoGetSource)
    , XAMP_LOAD_DLL_API(BASS_FX_TempoCreate)
    , XAMP_LOAD_DLL_API(BASS_FX_GetVersion) {
}
catch (const Exception& e) {
    XAMP_LOG_E(BASS_LIB.logger, "{}", e.GetErrorMessage());
}

std::string BassFxLib::GetName() const {
    return GetSharedLibraryName("bass_fx");
}

#ifdef XAMP_OS_WIN
BassCDLib::BassCDLib() try
    : module_(OpenSharedLibrary("basscd"))
    , XAMP_LOAD_DLL_API(BASS_CD_GetInfo)
    , XAMP_LOAD_DLL_API(BASS_CD_GetSpeed)
    , XAMP_LOAD_DLL_API(BASS_CD_Door)
    , XAMP_LOAD_DLL_API(BASS_CD_DoorIsLocked)
    , XAMP_LOAD_DLL_API(BASS_CD_DoorIsOpen)
    , XAMP_LOAD_DLL_API(BASS_CD_SetInterface)
    , XAMP_LOAD_DLL_API(BASS_CD_SetOffset)
    , XAMP_LOAD_DLL_API(BASS_CD_SetSpeed)
    , XAMP_LOAD_DLL_API(BASS_CD_Release)
    , XAMP_LOAD_DLL_API(BASS_CD_IsReady)
    , XAMP_LOAD_DLL_API(BASS_CD_GetID)
    , XAMP_LOAD_DLL_API(BASS_CD_GetTracks)
    , XAMP_LOAD_DLL_API(BASS_CD_GetTrackLength) {
}
catch (const Exception& e) {
    XAMP_LOG_E(BASS_LIB.logger, "{}", e.GetErrorMessage());
}
#endif

#ifdef XAMP_OS_WIN
std::string BassCDLib::GetName() const {
    return GetSharedLibraryName("basscd");
}
#endif

BassEncLib::BassEncLib()  try
    : module_(OpenSharedLibrary("bassenc"))
    , XAMP_LOAD_DLL_API(BASS_Encode_StartACMFile)
	, XAMP_LOAD_DLL_API(BASS_Encode_GetVersion)
	, XAMP_LOAD_DLL_API(BASS_Encode_GetACMFormat) {
}
catch (const Exception& e) {
    XAMP_LOG_E(BASS_LIB.logger, "{}", e.GetErrorMessage());
}

std::string BassEncLib::GetName() const {
    return GetSharedLibraryName("bassenc");
}

#ifdef XAMP_OS_WIN
BassAACEncLib::BassAACEncLib() try
    : module_(OpenSharedLibrary("bassenc_aac"))
    , XAMP_LOAD_DLL_API(BASS_Encode_AAC_StartFile)
    , XAMP_LOAD_DLL_API(BASS_Encode_AAC_GetVersion) {
}
catch (const Exception& e) {
    XAMP_LOG_E(BASS_LIB.logger, "{}", e.GetErrorMessage());
}

std::string BassAACEncLib::GetName() const {
    return GetSharedLibraryName("bass_aac");
}
#else
BassCAEncLib::BassCAEncLib() try
    : module_(OpenSharedLibrary("bassenc"))
    , XAMP_LOAD_DLL_API(BASS_Encode_StartCAFile) {
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

std::string BassCAEncLib::GetName() const {
    return GetSharedLibraryName("bassenc");
}
#endif

BassFLACEncLib::BassFLACEncLib() try
    : module_(OpenSharedLibrary("bassenc_flac"))
    , XAMP_LOAD_DLL_API(BASS_Encode_FLAC_StartFile)
	, XAMP_LOAD_DLL_API(BASS_Encode_FLAC_GetVersion) {
}
catch (const Exception& e) {
    XAMP_LOG_E(BASS_LIB.logger, "{}", e.GetErrorMessage());
}

std::string BassFLACEncLib::GetName() const {
    return GetSharedLibraryName("bassenc_flac");
}

BassLib::BassLib() try
    : logger(XampLoggerFactory.GetLogger(kBASSLoggerName))
    , module_(OpenSharedLibrary("bass"))
    , XAMP_LOAD_DLL_API(BASS_Init)
    , XAMP_LOAD_DLL_API(BASS_GetVersion)
    , XAMP_LOAD_DLL_API(BASS_SetConfig)
    , XAMP_LOAD_DLL_API(BASS_SetConfigPtr)
    , XAMP_LOAD_DLL_API(BASS_PluginLoad)
    , XAMP_LOAD_DLL_API(BASS_PluginGetInfo)
    , XAMP_LOAD_DLL_API(BASS_Free)
    , XAMP_LOAD_DLL_API(BASS_StreamCreateFile)
    , XAMP_LOAD_DLL_API(BASS_ChannelGetInfo)
    , XAMP_LOAD_DLL_API(BASS_StreamFree)
    , XAMP_LOAD_DLL_API(BASS_PluginFree)
    , XAMP_LOAD_DLL_API(BASS_ChannelGetData)
    , XAMP_LOAD_DLL_API(BASS_ChannelGetLength)
    , XAMP_LOAD_DLL_API(BASS_ChannelBytes2Seconds)
    , XAMP_LOAD_DLL_API(BASS_ChannelSeconds2Bytes)
    , XAMP_LOAD_DLL_API(BASS_ChannelSetPosition)
    , XAMP_LOAD_DLL_API(BASS_ChannelGetPosition)
    , XAMP_LOAD_DLL_API(BASS_ErrorGetCode)
    , XAMP_LOAD_DLL_API(BASS_ChannelGetAttribute)
    , XAMP_LOAD_DLL_API(BASS_ChannelSetAttribute)
    , XAMP_LOAD_DLL_API(BASS_StreamCreate)
    , XAMP_LOAD_DLL_API(BASS_StreamPutData)
    , XAMP_LOAD_DLL_API(BASS_ChannelSetFX)
    , XAMP_LOAD_DLL_API(BASS_ChannelRemoveFX)
    , XAMP_LOAD_DLL_API(BASS_FXSetParameters)
    , XAMP_LOAD_DLL_API(BASS_FXGetParameters)
    , XAMP_LOAD_DLL_API(BASS_StreamCreateURL)
    , XAMP_LOAD_DLL_API(BASS_StreamGetFilePosition)
    , XAMP_LOAD_DLL_API(BASS_ChannelIsActive)
	, XAMP_LOAD_DLL_API(BASS_SampleGetChannel) {
}
catch (const Exception& e) {
    XAMP_LOG_E(logger, "{}", e.GetErrorMessage());
}

BassLib::~BassLib() {
	if (!module_.is_valid()) {
        return;
	}
    Free();
}

std::string BassLib::GetName() const {
    return GetSharedLibraryName("bass");
}

HPLUGIN BassPluginLoadDeleter::invalid() noexcept {
    return 0;
}

 void BassPluginLoadDeleter::close(HPLUGIN value) {
     BASS_LIB.BASS_PluginFree(value);
}

HSTREAM BassStreamDeleter::invalid() noexcept {
    return 0;
}

void BassStreamDeleter::close(HSTREAM value) {
    BASS_LIB.BASS_StreamFree(value);
}

void BassLib::Load() {
    if (IsLoaded()) {
        return;
    }

#ifdef XAMP_OS_WIN
    // Disable 1ms timer resolution
#define BASS_CONFIG_NOTIMERES 29
    BASS_LIB.BASS_SetConfig(BASS_CONFIG_NOTIMERES, true);

    // Automatically reduce the read speed when a read error occurs?
    BASS_LIB.BASS_SetConfig(BASS_CONFIG_CD_AUTOSPEED, true);
    // Number of times to retry after a read error.
    BASS_LIB.BASS_SetConfig(BASS_CONFIG_CD_RETRY, 4);
    // Skip past errors?
    BASS_LIB.BASS_SetConfig(BASS_CONFIG_CD_SKIPERROR, false);
#endif

    BASS_LIB.BASS_Init(0, 44100, 0, nullptr, nullptr);
    XAMP_LOG_D(logger, "Load BASS_LIB {} successfully.", GetBassVersion(BASS_LIB.BASS_GetVersion()));
#ifdef XAMP_OS_WIN
    // Disable Media Foundation
    BASS_LIB.BASS_SetConfig(BASS_CONFIG_MF_DISABLE, true);
    BASS_LIB.BASS_SetConfig(BASS_CONFIG_MF_VIDEO, false);
    LoadPlugin("bass_aac.dll");
    LoadPlugin("bassflac.dll");
    LoadPlugin("bassape.dll");
    LoadPlugin("bassalac.dll");
    LoadPlugin("basscd.dll");
    // For GetSupportFileExtensions need!
    LoadPlugin("bassdsd.dll");
    LoadPlugin("bassopus.dll");
    LoadPlugin("basswebm.dll");
#else
    LoadPlugin("libbassflac.dylib");
    LoadPlugin("libbassdsd.dylib");
#endif

    BASS_LIB.BASS_SetConfig(BASS_CONFIG_DSD_FREQ, 88200);
    BASS_LIB.BASS_SetConfig(BASS_CONFIG_FLOATDSP, true);
    BASS_LIB.BASS_SetConfig(BASS_CONFIG_NET_TIMEOUT, 15 * 1000);
    BASS_LIB.BASS_SetConfig(BASS_CONFIG_NET_BUFFER, 50000);
    BASS_LIB.BASS_SetConfig(BASS_CONFIG_NET_PREBUF, 80);
    BASS_LIB.BASS_SetConfig(BASS_CONFIG_NET_RESTRATE, 1024 * 1024);
    BASS_LIB.BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, false);
    BASS_LIB.BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);
    BASS_LIB.BASS_SetConfig(BASS_CONFIG_ASYNCFILE_BUFFER, 65536);
    BASS_LIB.BASS_SetConfigPtr(BASS_CONFIG_NET_AGENT, String::ToStdWString(XAMP_HTTP_USER_AGENT).c_str());
}

void BassLib::Free() {
    plugins_.clear();
    if (module_.is_valid()) {
        try {
            BASS_LIB.BASS_Free();
        }
        catch (...) {
        }
    }
}

void BassLib::LoadPlugin(const std::string & file_name) {
    const auto plugin_fully_path = GetComponentsFilePath() / Path(file_name);
    BassPluginHandle plugin(BASS_LIB.BASS_PluginLoad(plugin_fully_path.string().c_str(), 0));
    if (!plugin) {
        XAMP_LOG_D(logger, "Load {} failure. error:{}",
            file_name,
            BASS_LIB.BASS_ErrorGetCode());
        return;
    }

    const auto* info = BASS_LIB.BASS_PluginGetInfo(plugin.get());
    XAMP_LOG_D(logger, "Load {} {} successfully.", file_name, GetBassVersion(info->version));

    plugins_[file_name] = std::move(plugin);
}

OrderedMap<std::string, std::string> BassLib::GetPluginVersion() const {
    OrderedMap<std::string, std::string> vers;

    for (const auto& [key, value] : plugins_) {
        const auto* info = BASS_LIB.BASS_PluginGetInfo(value.get());
        vers[key] = GetBassVersion(info->version);
    }
    return vers;
}

void BassLib::LoadVersionInfo() {
    dll_versions_ = GetPluginVersion();
    dll_versions_[BASS_LIB.GetName()] = GetBassVersion(BASS_LIB.BASS_GetVersion());
    dll_versions_[BASS_LIB.MixLib->GetName()] = GetBassVersion(BASS_LIB.MixLib->BASS_Mixer_GetVersion());
    dll_versions_[BASS_LIB.FxLib->GetName()] = GetBassVersion(BASS_LIB.FxLib->BASS_FX_GetVersion());
    if (BASS_LIB.EncLib != nullptr) {
        dll_versions_[BASS_LIB.EncLib->GetName()] = GetBassVersion(BASS_LIB.EncLib->BASS_Encode_GetVersion());
    }
    dll_versions_[BASS_LIB.FLACEncLib->GetName()] = GetBassVersion(BASS_LIB.FLACEncLib->BASS_Encode_FLAC_GetVersion());
}

OrderedMap<std::string, std::string> BassLib::GetVersions() const {
    return dll_versions_;
}

HashSet<std::string> BassLib::GetSupportFileExtensions() const {
    HashSet<std::string> result;
	
	for (const auto& [key, value] : plugins_) {
        const auto* info = BASS_LIB.BASS_PluginGetInfo(value.get());
		
        for (DWORD i = 0; i < info->formatc; ++i) {
            XAMP_LOG_T(logger, "Load BASS_LIB {} {}", info->formats[i].name, info->formats[i].exts);
        	for (auto file_ext : String::Split(info->formats[i].exts, ";")) {
                std::string ext(file_ext);
                auto pos = ext.find('*');
        		if (pos != std::string::npos) {
                    ext.erase(pos, 1);
        		}                
                result.insert(ext);
        	}           
        }
	}

	// Workaround!
    result.insert(".wav");
    result.insert(".mp3");

    #ifdef XAMP_OS_MAC
    result.insert(".m4a");
    result.insert(".aac");
    #endif

    return result;
} 

XAMP_STREAM_NAMESPACE_END
