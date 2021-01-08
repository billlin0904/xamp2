#include <base/singleton.h>
#include <base/str_utilts.h>
#include <stream/basslib.h>

namespace xamp::stream {

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
#ifdef XAMP_OS_WIN
    : module_(LoadModule("bassdsd.dll"))
#else
    : module_(LoadModule("libbassdsd.dylib"))
#endif
    , BASS_DSD_StreamCreateFile(module_, "BASS_DSD_StreamCreateFile") {
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

BassMixLib::BassMixLib() try
#ifdef XAMP_OS_WIN
    : module_(LoadModule("bassmix.dll"))
#else
    : module_(LoadModule("libbassmix.dylib"))
#endif
    , BASS_Mixer_StreamCreate(module_, "BASS_Mixer_StreamCreate")
    , BASS_Mixer_StreamAddChannel(module_, "BASS_Mixer_StreamAddChannel")
    , BASS_Mixer_GetVersion(module_, "BASS_Mixer_GetVersion") {
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

BassLib::BassLib() try
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
    , BASS_ChannelSetAttribute(module_, "BASS_ChannelSetAttribute")
    , BASS_StreamCreate(module_, "BASS_StreamCreate")
    , BASS_StreamPutData(module_, "BASS_StreamPutData")
    , BASS_ChannelSetFX(module_, "BASS_ChannelSetFX")
    , BASS_ChannelRemoveFX(module_, "BASS_ChannelRemoveFX")
    , BASS_FXSetParameters(module_, "BASS_FXSetParameters")
    , BASS_FXGetParameters(module_, "BASS_FXGetParameters") {
}
catch (const Exception& e) {
    XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

HPLUGIN BassPluginLoadDeleter::invalid() noexcept {
    return 0;
}

 void BassPluginLoadDeleter::close(HPLUGIN value) {
    Singleton<BassLib>::GetInstance().BASS_PluginFree(value);
}

HSTREAM BassStreamDeleter::invalid() noexcept {
    return 0;
}

void BassStreamDeleter::close(HSTREAM value) {
    Singleton<BassLib>::GetInstance().BASS_StreamFree(value);
}

void BassLib::Load() {
    if (IsLoaded()) {
        return;
    }

    Singleton<BassLib>::GetInstance().BASS_Init(0, 44100, 0, nullptr, nullptr);
#ifdef XAMP_OS_WIN
    // Disable Media Foundation
    Singleton<BassLib>::GetInstance().BASS_SetConfig(BASS_CONFIG_MF_DISABLE, true);
    Singleton<BassLib>::GetInstance().BASS_SetConfig(BASS_CONFIG_MF_VIDEO, false);
    LoadPlugin("bass_aac.dll");
    LoadPlugin("bassflac.dll");
    LoadPlugin("bass_ape.dll");
#endif
    Singleton<BassLib>::GetInstance().BASS_SetConfig(BASS_CONFIG_FLOATDSP, true);
}

void BassLib::Free() {
    XAMP_LOG_INFO("Release BassLib dll.");
    plugins_.clear();
    if (module_.is_valid()) {
        try {
            Singleton<BassLib>::GetInstance().BASS_Free();
        }
        catch (const Exception& e) {
            XAMP_LOG_INFO("{}", e.what());
        }
    }
}

void BassLib::LoadPlugin(std::string const & file_name) {
    BassPluginHandle plugin(Singleton<BassLib>::GetInstance()
                                .BASS_PluginLoad(file_name.c_str(), 0));
    if (!plugin) {
        XAMP_LOG_DEBUG("Load {} failure. error:{}",
                       file_name,
                       Singleton<BassLib>::GetInstance().BASS_ErrorGetCode());
        return;
    }

    const auto* info = Singleton<BassLib>::GetInstance().BASS_PluginGetInfo(plugin.get());
    XAMP_LOG_DEBUG("Load {} {} successfully.", file_name, GetBassVersion(info->version));

    plugins_[file_name] = std::move(plugin);
}

std::set<std::string> BassLib::GetSupportFileExtensions() const {
    std::set<std::string> result;
	
	for (const auto& [key, value] : plugins_) {
        const auto* info = Singleton<BassLib>::GetInstance().BASS_PluginGetInfo(value.get());
		
        for (auto i = 0; i < info->formatc; ++i) {
        	for (auto file_ext : Split(info->formats[i].exts, ";")) {
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
    result.insert(".dff");
    result.insert(".dsf");
	
    return result;
} 

}
