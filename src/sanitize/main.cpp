#include <iostream>

#include <base/scopeguard.h>
#include <base/str_utilts.h>
#include <output_device/win32/exclusivewasapidevicetype.h>
#include <player/api.h>

using namespace xamp;
using namespace base;
using namespace player;
using namespace output_device;
using namespace win32;

void TestPlayDSD(std::wstring const &file_path) {
	auto player = MakeAudioPlayer(std::weak_ptr<IPlaybackStateAdapter>());
	player->Open(file_path, ExclusiveWasapiDeviceType::Id);
	player->PrepareToPlay();
	player->Play();
	std::cin.get();
	player->Stop();
}

int main(int argc, char* argv[]) {
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
	TestPlayDSD(String::ToStdWString(argv[1]));
}
