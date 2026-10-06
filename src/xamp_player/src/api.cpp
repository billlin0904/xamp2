#include <player/api.h>

#include <base/threadpool.h>
#include <base/threadpoolbuilder.h>
#include <base/fftlib.h>
#include <base/logger.h>
#include <base/charset_detector.h>
#include <base/furigana.h>
#include <base/zib_util.h>

#include <stream/api.h>
#include <stream/icddevice.h>

#include <player/audio_player.h>

#include <metadata/api.h>
#include <stream/mbdiscid.h>

#include <cstddef>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

namespace {

struct RequiredComponentLoader {
    const char* name;
    void (*load)();
};

void loadRequiredComponent(const char* name, void (*loader)()) {
    try {
        loader();
        XAMP_LOG_DEBUG("load {} lib success.", name);
    }
    catch (...) {
        XAMP_LOG_ERROR("load {} lib failed.", name);
        throw;
    }
}

constexpr RequiredComponentLoader kComponentLoaders[] {
    { "Bass", loadBassLib },
    { "Mqa", loadMqaLib },
    { "Src", loadSrcLib },
    { "Deflate", loadLibdeflate },
#if defined(XAMP_OS_WIN) || defined(XAMP_OS_LINUX)
    { "Fft", loadFftLib },
#endif
    { "AvLib", loadAvLib },
    { "Soxr", loadSoxrLib },
    { "LibCue", loadCueLib },
    { "UcharDect", loadUcharDectLib },
    { "Furigana", loadFuriganaDll },
#ifdef XAMP_OS_WIN
    { "R8Brain", loadR8BrainLib },
    { "MBDiscId", loadMBDiscIdLib },
#endif
};

} // namespace

ComponentSharedLibraryLoader::ComponentSharedLibraryLoader() {
}

ComponentSharedLibraryLoader::~ComponentSharedLibraryLoader() {
	unload();
}

void ComponentSharedLibraryLoader::load() {
    for (const auto& loader : kComponentLoaders) {
        loadRequiredComponent(loader.name, loader.load);
        if (loader.load == loadBassLib) {
            // Keep cleanup valid even if a later component fails to load.
            bass_loaded_ = true;
        }
    }
}

void ComponentSharedLibraryLoader::unload() {
    if (bass_loaded_) {
        bass_loaded_ = false;
        unloadBassLib();
    }
}

#ifdef XAMP_OS_WIN
ScopedPtr<ICDDevice> openCD(int32_t driver_letter) {
    return StreamFactory::makeCDDevice(driver_letter);
}
#endif

std::shared_ptr<IAudioPlayer> makeAudioPlayer() {
	return std::make_shared<AudioPlayer>(
        ThreadPoolBuilder::makePlaybackThreadPool(),
        ThreadPoolBuilder::makePlayerThreadPool());
}

XAMP_AUDIO_PLAYER_NAMESPACE_END
