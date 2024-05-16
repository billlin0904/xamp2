#include <player/api.h>

#include <base/ithreadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/text_encoding.h>
#include <base/charset_detector.h>

#include <stream/api.h>
#include <stream/icddevice.h>

#include <player/audio_player.h>

#include <metadata/api.h>

#include <stream/ebur128reader.h>
#include <stream/mbdiscid.h>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

void LoadComponentSharedLibrary() {   
    LoadBassLib();
    XAMP_LOG_DEBUG("Load BASS lib success.");

    LoadSoxrLib();
    XAMP_LOG_DEBUG("Load Soxr lib success.");

    LoadSrcLib();
    XAMP_LOG_DEBUG("Load Src lib success.");

    LoadFFTLib();
    XAMP_LOG_DEBUG("Load FFT lib success.");

    LoadAvLib();
    XAMP_LOG_DEBUG("Load avlib success.");

    LoadEbur128Lib();
    XAMP_LOG_DEBUG("Load ebur128 lib success.");

    LoadCueLib();
    XAMP_LOG_DEBUG("Load libcue lib success.");

    LoadUcharDectLib();
    XAMP_LOG_DEBUG("Load uchardect lib success.");

    LoadLibIconvLib();
    XAMP_LOG_DEBUG("Load iconv lib success.");

    LoadChromaprintLib();
    XAMP_LOG_DEBUG("Load chromaprint lib success.");

#ifdef XAMP_OS_WIN
    LoadR8brainLib();
    XAMP_LOG_DEBUG("Load r8brain lib success.");

    LoadMBDiscIdLib();
    XAMP_LOG_DEBUG("Load mbdiscid lib success.");

    GetPlaybackThreadPool();
    XAMP_LOG_DEBUG("Start Playback thread pool success.");

    GetOutputDeviceThreadPool();
    XAMP_LOG_DEBUG("Start WASAPI thread pool success.");
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

XAMP_AUDIO_PLAYER_NAMESPACE_END
