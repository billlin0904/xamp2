#include <base/singleton.h>
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

HPLUGIN BassPluginLoadTraits::invalid() noexcept {
    return 0;
}

 void BassPluginLoadTraits::close(HPLUGIN value) {
    Singleton<BassLib>::GetInstance().BASS_PluginFree(value);
}

HSTREAM BassStreamTraits::invalid() noexcept {
    return 0;
}

void BassStreamTraits::close(HSTREAM value) {
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

    const auto info = Singleton<BassLib>::GetInstance().BASS_PluginGetInfo(plugin.get());
    const uint32_t major_version(HiByte(HiWord(info->version)));
    const uint32_t minor_version(LowByte(HiWord(info->version)));
    const uint32_t micro_version(HiByte(LoWord(info->version)));
    const uint32_t build_version(LowByte(LoWord(info->version)));

    std::ostringstream ostr;
    ostr << major_version << "."
         << minor_version << "."
         << micro_version << "."
         << build_version;
    XAMP_LOG_DEBUG("Load {} {} successfully.", file_name, ostr.str());

    plugins_[file_name] = std::move(plugin);
}

}
