#include "gtest/gtest.h"

#include <output_device/devicefactory.h>
#include <player/audio_player.h>

using namespace xamp::player;

TEST(UnitTest, PlayerOpenStreamTest) {
    if (auto default_device = DeviceFactory::Instance().CreateDefaultDevice()) {
        auto device_info = default_device.value()->GetDefaultDeviceInfo();
        auto player = std::make_shared<AudioPlayer>();
        player->Open(L"2.flac", false, device_info);
        player->SetVolume(player->GetVolume());
        player->PlayStream();
        player->Stop();
    }
}
