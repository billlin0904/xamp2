#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/ithreadpool.h>

#include <output_device/api.h>

#include <stream/icddevice.h>
#include <stream/fftwlib.h>
#include <player/audio_player.h>
#include <stream/soxresampler.h>
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

void XStartup() {
    LoadBassLib();
    XAMP_LOG_DEBUG("Load BASS dll success.");

    LoadSoxrLib();
    XAMP_LOG_DEBUG("Load Soxr dll success.");

    PreventSleep(true);

#ifdef XAMP_OS_WIN
    LoadFFTLib();
    XAMP_LOG_DEBUG("Load FFT dll success.");

    WASAPIThreadPool();
    XAMP_LOG_DEBUG("Start WASAPI thread pool success.");
#endif

    PlaybackThreadPool();
    XAMP_LOG_DEBUG("Start Playback thread pool success.");
}


}
