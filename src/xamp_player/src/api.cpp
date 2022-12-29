#include <player/api.h>

#include <base/ithreadpoolexecutor.h>
#include <base/logger_impl.h>

#include <stream/api.h>
#include <stream/icddevice.h>

#include <player/audio_player.h>

#include <player/ebur128reader.h>
#include <player/mbdiscid.h>

namespace xamp::player {

void LoadComponentSharedLibrary() {
    GetPlaybackThreadPool();
    XAMP_LOG_DEBUG("Start Playback thread pool success.");

    LoadBassLib();
    XAMP_LOG_DEBUG("Load BASS lib success.");

    LoadSoxrLib();
    XAMP_LOG_DEBUG("Load Soxr lib success.");

    LoadFFTLib();
    XAMP_LOG_DEBUG("Load FFT lib success.");

    LoadAvLib();
    XAMP_LOG_DEBUG("Load avlib success.");

    Ebur128Reader::LoadEbur128Lib();
    XAMP_LOG_DEBUG("Load ebur128 lib success.");

#ifdef XAMP_OS_WIN
    GetWASAPIThreadPool();
    XAMP_LOG_DEBUG("Start WASAPI thread pool success.");

    LoadR8brainLib();
    XAMP_LOG_DEBUG("Load r8brain lib success.");

    MBDiscId::LoadMBDiscIdLib();
    XAMP_LOG_DEBUG("Load mbdiscid lib success.");
#endif
}

#ifdef XAMP_OS_WIN
AlignPtr<ICDDevice> OpenCD(int32_t driver_letter) {
    return StreamFactory::MakeCDDevice(driver_letter);
}
#endif

std::shared_ptr<IAudioPlayer> MakeAudioPlayer() {
    return MakeAlignedShared<AudioPlayer>();
}

}
