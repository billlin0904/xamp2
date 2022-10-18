#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/ithreadpool.h>

#include <output_device/api.h>

#include <stream/icddevice.h>
#include <stream/soxresampler.h>
#include <stream/fftwlib.h>
#include <player/audio_player.h>
#include <player/ebur128replaygain_scanner.h>
#include <player/mbdiscid.h>
#include <player/audio_util.h>

#include <player/api.h>

namespace xamp::player {

#ifdef XAMP_OS_WIN
AlignPtr<ICDDevice> OpenCD(int32_t driver_letter) {
    return DspComponentFactory::MakeCDDevice(driver_letter);
}
#endif

std::shared_ptr<IAudioPlayer> MakeAudioPlayer(const std::weak_ptr<IPlaybackStateAdapter>& adapter) {
    return MakeAlignedShared<AudioPlayer>(adapter);
}

XampIniter::XampIniter() {
}

void XampIniter::Init() {
    if (success) {
        return;
    }

    GetPlaybackThreadPool();
    XAMP_LOG_DEBUG("Start Playback thread pool success.");

#ifdef XAMP_OS_WIN
    GetWASAPIThreadPool();
    XAMP_LOG_DEBUG("Start WASAPI thread pool success.");
#endif

    PreventSleep(true);
    success = true;
}

void XampIniter::LoadLib() {
    LoadBassLib();
    XAMP_LOG_DEBUG("Load BASS lib success.");

    LoadSoxrLib();
    XAMP_LOG_DEBUG("Load Soxr lib success.");

    LoadFFTLib();
    XAMP_LOG_DEBUG("Load FFT lib success.");
#ifdef XAMP_OS_WIN
    LoadR8brainLib();
    XAMP_LOG_DEBUG("Load r8brain lib success.");

    MBDiscId::LoadMBDiscIdLib();
    XAMP_LOG_DEBUG("Load mbdiscid lib success.");
#endif

    Ebur128ReplayGainScanner::LoadEbur128Lib();
    XAMP_LOG_DEBUG("Load ebur128 lib success.");
}

XampIniter::~XampIniter() {
    if (!success) {
        return;
    }
    GetPlaybackThreadPool().Stop();
#ifdef XAMP_OS_WIN
    GetWASAPIThreadPool().Stop();
#endif
    PreventSleep(false);
}

}
