#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/ithreadpool.h>

#include <output_device/api.h>

#include <stream/bassfilestream.h>
#include <stream/icddevice.h>

#include <player/audio_player.h>
#include <player/soxresampler.h>
#include <player/chromaprint.h>
#include <player/audio_util.h>

#include <player/api.h>

namespace xamp::player {

#ifdef XAMP_OS_WIN
AlignPtr<ICDDevice> OpenCD(int32_t driver_letter) {
    return MakeCDDevice(driver_letter);
}
#endif

std::shared_ptr<IAudioPlayer> MakeAudioPlayer(const std::weak_ptr<IPlaybackStateAdapter>& adapter) {
    return MakeAlignedShared<AudioPlayer>(adapter);
}

void Xamp2Startup() {
    LoadBassLib();
    XAMP_LOG_DEBUG("Load BASS dll success.");

    LoadSoxrLib();
    XAMP_LOG_DEBUG("Load Soxr dll success.");

    try {
        Chromaprint::LoadChromaprintLib();
        XAMP_LOG_DEBUG("Load Chromaprint dll success.");
    }
    catch (...) {
        // Ignore exception.
    }

    PreventSleep(true);

#ifdef XAMP_OS_WIN
    WASAPIThreadPool();
#endif
    PlaybackThreadPool();
}


}
