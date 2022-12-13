#include <player/api.h>

#include <stream/api.h>
#include <stream/icddevice.h>
#include <player/audio_player.h>

namespace xamp::player {

#ifdef XAMP_OS_WIN
AlignPtr<ICDDevice> OpenCD(int32_t driver_letter) {
    return StreamFactory::MakeCDDevice(driver_letter);
}
#endif

std::shared_ptr<IAudioPlayer> MakeAudioPlayer() {
    return MakeAlignedShared<AudioPlayer>();
}

}
