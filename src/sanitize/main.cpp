#include <iostream>
#include <base/scopeguard.h>
#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/asiodevicetype.h>
#include <player/audio_player.h>

int main() {
	using namespace xamp;
	using namespace player;
	using namespace base;
	using namespace output_device;
	using namespace win32;

	Logger::GetInstance()
		.AddDebugOutputLogger()
		.AddFileLogger("xamp.log")
		.GetLogger("xamp");

	XAMP_ON_SCOPE_EXIT(
		Logger::GetInstance().Shutdown();
	);

	AudioPlayer::Initialize();

	auto player = MakeAlignedShared<AudioPlayer>();
	auto avaiable_device_type = player->GetAudioDeviceManager().GetAvailableDeviceType();

	player->Open("C:\\Users\\rdbill0452\\Music\\Test\\DSD.dsf", ExclusiveWasapiDeviceType::Id);
	player->PrepareToPlay();
	player->Play();
	std::cin.get();
	player->Stop();
}
