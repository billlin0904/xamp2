#include <iostream>
#include <base/scopeguard.h>
#include <output_device/asiodevicetype.h>
#include <player/audio_player.h>

int main()
{
	using namespace xamp;
	using namespace xamp::player;
	using namespace xamp::base;
	using namespace xamp::output_device;

	Logger::GetInstance()
		.AddDebugOutputLogger()
		.AddFileLogger("xamp.log")
		.GetLogger("xamp");

	XAMP_ON_SCOPE_EXIT(
		Logger::GetInstance().Shutdown();
	);

	AudioPlayer::LoadDecoder();

	auto player = std::make_shared<AudioPlayer>();
	player->Open("C:\\Users\\rdbill0452\\Music\\Test\\DSD.dsf", ASIODeviceType::Id);
	player->PrepareToPlay();
	player->Play();
	std::cin.get();
	player->Stop();
}
