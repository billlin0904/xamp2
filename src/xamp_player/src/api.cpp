#include <player/api.h>

#include <base/threadpool.h>
#include <base/fftlib.h>
#include <base/logger.h>
#include <base/charset_detector.h>
#include <base/furigana.h>

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

void LoadRequiredComponent(const char* name, void (*loader)()) {
    try {
        loader();
        XAMP_LOG_DEBUG("load {} lib success.", name);
    }
    catch (...) {
        XAMP_LOG_ERROR("load {} lib failed.", name);
        throw;
    }
}

template <size_t Size>
void LoadRequiredComponents(const RequiredComponentLoader (&loaders)[Size]) {
    for (const auto& loader : loaders) {
        LoadRequiredComponent(loader.name, loader.load);
    }
}

constexpr RequiredComponentLoader kComponentLoaders[] {
    { "BASS", LoadBassLib },
    { "MQA", LoadMqaLib },
    { "Src", LoadSrcLib },
#if defined(XAMP_OS_WIN) || defined(XAMP_OS_LINUX)
    { "FFT", LoadFFTLib },
#endif
    { "avlib", LoadAvLib },
    { "Soxr", LoadSoxrLib },
    { "libcue", LoadCueLib },
    { "uchardect", LoadUcharDectLib },
    { "furigana", LoadFuriganaDll },
#ifdef XAMP_OS_WIN
    { "r8brain", LoadR8brainLib },
    { "mbdiscid", LoadMBDiscIdLib },
#endif
};

} // namespace

void LoadComponentSharedLibrary() {
    LoadRequiredComponents(kComponentLoaders);
}

#ifdef XAMP_OS_WIN
ScopedPtr<ICDDevice> OpenCD(int32_t driver_letter) {
    return StreamFactory::makeCDDevice(driver_letter);
}
#endif

std::shared_ptr<IAudioPlayer> MakeAudioPlayer() {
	return std::make_shared<AudioPlayer>(
        ThreadPoolBuilder::makePlaybackThreadPool(),
        ThreadPoolBuilder::makePlayerThreadPool());
}

XAMP_AUDIO_PLAYER_NAMESPACE_END
