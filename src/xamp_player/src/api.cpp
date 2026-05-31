#include <player/api.h>

#include <base/ithreadpoolexecutor.h>
#include <base/fftlib.h>
#include <base/logger.h>
#include <base/charset_detector.h>
#include <base/furigana.h>

#include <stream/api.h>
#include <stream/icddevice.h>

#include <player/audio_player.h>

#include <metadata/api.h>
#include <stream/mbdiscid.h>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

namespace {

void LoadRequiredComponent(const char* name, void (*loader)()) {
    try {
        loader();
        XAMP_LOG_DEBUG("Load {} lib success.", name);
    }
    catch (...) {
        XAMP_LOG_ERROR("Load {} lib failed.", name);
        throw;
    }
}

} // namespace

void LoadComponentSharedLibrary() {
    LoadRequiredComponent("BASS", LoadBassLib);
    LoadRequiredComponent("MQA", LoadMqaLib);
    LoadRequiredComponent("Src", LoadSrcLib);
#if defined(XAMP_OS_WIN) || defined(XAMP_OS_LINUX)
    LoadRequiredComponent("FFT", LoadFFTLib);
#endif
    LoadRequiredComponent("avlib", LoadAvLib);
    LoadRequiredComponent("Soxr", LoadSoxrLib);
    LoadRequiredComponent("libcue", LoadCueLib);
    LoadRequiredComponent("uchardect", LoadUcharDectLib);
    LoadRequiredComponent("furigana", LoadFuriganaDll);
#ifdef XAMP_OS_WIN
    LoadRequiredComponent("r8brain", LoadR8brainLib);
    LoadRequiredComponent("mbdiscid", LoadMBDiscIdLib);
#endif
}

#ifdef XAMP_OS_WIN
ScopedPtr<ICDDevice> OpenCD(int32_t driver_letter) {
    return StreamFactory::MakeCDDevice(driver_letter);
}
#endif

std::shared_ptr<IAudioPlayer> MakeAudioPlayer() {
	return std::make_shared<AudioPlayer>(
        ThreadPoolBuilder::MakePlaybackThreadPool(),
        ThreadPoolBuilder::MakePlayerThreadPool());
}

XAMP_AUDIO_PLAYER_NAMESPACE_END
