#include <iostream>

#include <base/scopeguard.h>
#include <output_device/win32/exclusivewasapidevicetype.h>
#include <player/api.h>

using namespace xamp;
using namespace base;
using namespace player;
using namespace output_device;
using namespace win32;

void TestPlayDSD() {
	auto player = MakeAudioPlayer(std::weak_ptr<IPlaybackStateAdapter>());
	player->Open("C:\\Users\\rdbill0452\\Music\\Test\\DSD.dsf", ExclusiveWasapiDeviceType::Id);
	player->PrepareToPlay();
	player->Play();
	std::cin.get();
	player->Stop();
}

int main() {	
	Logger::GetInstance()
		.AddDebugOutputLogger()
		.AddConsoleLogger()
		.AddFileLogger("xamp.log")
		.GetLogger("xamp");

	XAMP_SET_LOG_LEVEL(spdlog::level::trace);

	XAMP_ON_SCOPE_EXIT(
		Logger::GetInstance().Shutdown();
	);

	Xamp2Startup();
	TestPlayDSD();
}
