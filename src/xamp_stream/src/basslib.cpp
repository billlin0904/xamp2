#include <stream/basslib.h>

#include <base/logger.h>
#include <base/str_utilts.h>

#include <bass/bassdsd.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(BASS);

template <typename t>
constexpr uint8_t HiByte(t val) {
    return static_cast<uint8_t>(val >> 8);
}

template <typename t>
constexpr uint8_t LowByte(t val) {
    return static_cast<uint8_t>(val);
}

template <typename t>
constexpr uint16_t HiWord(t val) {
    return static_cast<uint16_t>((static_cast<uint32_t>(val) >> 16) & 0xFFFF);
}

template <typename t>
constexpr uint16_t LoWord(t val) {
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
    : module_(openSharedLibrary("bassdsd"))
    , XAMP_LOAD_DLL_API(BASS_DSD_StreamCreateFile)
    , XAMP_LOAD_DLL_API(BASS_DSD_StreamCreateFileUser) {
}
catch (const Exception& e) {
    XAMP_LOG_E(LIB_BASS.logger, "{}", e.getErrorMessage());
}

BassMixLib::BassMixLib() try
    : module_(openSharedLibrary("bassmix"))
    , XAMP_LOAD_DLL_API(BASS_Mixer_StreamCreate)
    , XAMP_LOAD_DLL_API(BASS_Mixer_StreamAddChannel)
    , XAMP_LOAD_DLL_API(BASS_Mixer_GetVersion) {
}
catch (const Exception& e) {
    XAMP_LOG_E(LIB_BASS.logger, "{}", e.getErrorMessage());
}

std::string BassMixLib::getName() const {
    return getSharedLibraryName("bassmix");
}

BassFxLib::BassFxLib() try
    : module_(openSharedLibrary("bass_fx"))
    , XAMP_LOAD_DLL_API(BASS_FX_TempoGetSource)
    , XAMP_LOAD_DLL_API(BASS_FX_TempoCreate)
    , XAMP_LOAD_DLL_API(BASS_FX_GetVersion) {
}
catch (const Exception& e) {
    XAMP_LOG_E(LIB_BASS.logger, "{}", e.getErrorMessage());
}

std::string BassFxLib::getName() const {
    return getSharedLibraryName("bass_fx");
}

#ifdef XAMP_OS_WIN
BassCDLib::BassCDLib() try
    : module_(openSharedLibrary("basscd"))
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
    XAMP_LOG_E(LIB_BASS.logger, "{}", e.getErrorMessage());
}
#endif

#ifdef XAMP_OS_WIN
std::string BassCDLib::getName() const {
    return getSharedLibraryName("basscd");
}
#endif

BassLib::BassLib() try
    : logger(XampLoggerFactory.getLogger(kBASSLoggerName))
    , module_(openSharedLibrary("bass"))
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
	, XAMP_LOAD_DLL_API(BASS_SampleGetChannel)
    , XAMP_LOAD_DLL_API(BASS_StreamCreateFileUser) {
}
catch (const Exception& e) {
    XAMP_LOG_E(logger, "{}", e.getErrorMessage());
}

BassLib::~BassLib() {
    XAMP_LOG_E(logger, "destroy BASS library.");

	if (!module_.is_valid()) {
        return;
	}
    Free();
}

std::string BassLib::getName() const {
    return getSharedLibraryName("bass");
}

HPLUGIN BassPluginLoadDeleter::invalid() {
    return 0;
}

 void BassPluginLoadDeleter::close(HPLUGIN value) {
     LIB_BASS.BASS_PluginFree(value);
}

HSTREAM BassStreamDeleter::invalid() {
    return 0;
}

void BassStreamDeleter::close(HSTREAM value) {
    LIB_BASS.BASS_StreamFree(value);
}

void BassLib::load() {
    if (IsLoaded()) {
        return;
    }

#ifdef XAMP_OS_WIN
    // Disable 1ms timer resolution
#define BASS_CONFIG_NOTIMERES 29
    LIB_BASS.BASS_SetConfig(BASS_CONFIG_NOTIMERES, true);

    // Automatically reduce the read speed when a read error occurs?
    LIB_BASS.BASS_SetConfig(BASS_CONFIG_CD_AUTOSPEED, true);
    // Number of times to retry after a read error.
    LIB_BASS.BASS_SetConfig(BASS_CONFIG_CD_RETRY, 4);
    // Skip past errors?
    LIB_BASS.BASS_SetConfig(BASS_CONFIG_CD_SKIPERROR, false);
#endif

    LIB_BASS.BASS_Init(0, 44100, 0, nullptr, nullptr);
    XAMP_LOG_D(logger, "load BASS_LIB {} successfully.", GetBassVersion(LIB_BASS.BASS_GetVersion()));
#ifdef XAMP_OS_WIN
    // Disable Media Foundation
    LIB_BASS.BASS_SetConfig(BASS_CONFIG_MF_DISABLE, true);
    LIB_BASS.BASS_SetConfig(BASS_CONFIG_MF_VIDEO, false);
    loadPlugin("bass_aac.dll");
    loadPlugin("bassflac.dll");
    loadPlugin("bassape.dll");
    loadPlugin("bassalac.dll");
    loadPlugin("basscd.dll");
    // For getSupportFileExtensions need!
    loadPlugin("bassdsd.dll");
    loadPlugin("bassopus.dll");
    loadPlugin("basswebm.dll");
#else
    loadPlugin(getSharedLibraryName("bassflac"));
    loadPlugin(getSharedLibraryName("bassdsd"));
#endif

    LIB_BASS.BASS_SetConfig(BASS_CONFIG_DSD_FREQ, 88200);
    LIB_BASS.BASS_SetConfig(BASS_CONFIG_FLOATDSP, true);
    LIB_BASS.BASS_SetConfig(BASS_CONFIG_NET_TIMEOUT, 15 * 1000);
    LIB_BASS.BASS_SetConfig(BASS_CONFIG_NET_BUFFER, 50000);
    LIB_BASS.BASS_SetConfig(BASS_CONFIG_NET_PREBUF, 80);
    LIB_BASS.BASS_SetConfig(BASS_CONFIG_NET_RESTRATE, 1024 * 1024);
    LIB_BASS.BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, false);
    LIB_BASS.BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);
    LIB_BASS.BASS_SetConfig(BASS_CONFIG_ASYNCFILE_BUFFER, 65536);
    LIB_BASS.BASS_SetConfigPtr(BASS_CONFIG_NET_AGENT, String::toStdWString(XAMP_HTTP_USER_AGENT).c_str());
}

void BassLib::Free() {
    plugins_.clear();
    if (module_.is_valid()) {
        try {
            LIB_BASS.BASS_Free();
        }
        catch (...) {
        }
    }
}

void BassLib::loadPlugin(const std::string & file_name) {
    const auto plugin_fully_path = getComponentsFilePath() / Path(file_name);
    BassPluginHandle plugin(LIB_BASS.BASS_PluginLoad(plugin_fully_path.string().c_str(), 0));
    if (!plugin) {
        XAMP_LOG_D(logger, "load {} failure. error:{}",
            file_name,
            LIB_BASS.BASS_ErrorGetCode());
        return;
    }

    const auto* info = LIB_BASS.BASS_PluginGetInfo(plugin.get());
    XAMP_LOG_D(logger, "load {} {} successfully.", file_name, GetBassVersion(info->version));

    plugins_[file_name] = std::move(plugin);
}

OrderedMap<std::string, std::string> BassLib::getPluginVersion() const {
    OrderedMap<std::string, std::string> vers;

    for (const auto& [key, value] : plugins_) {
        const auto* info = LIB_BASS.BASS_PluginGetInfo(value.get());
        vers[key] = GetBassVersion(info->version);
    }
    return vers;
}

void BassLib::loadVersionInfo() {
    dll_versions_ = getPluginVersion();
    dll_versions_[LIB_BASS.getName()] = GetBassVersion(LIB_BASS.BASS_GetVersion());
    dll_versions_[LIB_BASS.MixLib->getName()] = GetBassVersion(LIB_BASS.MixLib->BASS_Mixer_GetVersion());
    dll_versions_[LIB_BASS.FxLib->getName()] = GetBassVersion(LIB_BASS.FxLib->BASS_FX_GetVersion());
}

OrderedMap<std::string, std::string> BassLib::getVersions() const {
    return dll_versions_;
}

HashSet<std::string> BassLib::getSupportFileExtensions() const {
    HashSet<std::string> result;
	
	for (const auto& [key, value] : plugins_) {
        const auto* info = LIB_BASS.BASS_PluginGetInfo(value.get());
		
        for (DWORD i = 0; i < info->formatc; ++i) {
            XAMP_LOG_T(logger, "load BASS_LIB {} {}", info->formats[i].name, info->formats[i].exts);
        	for (auto file_ext : String::split(info->formats[i].exts, ";")) {
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
