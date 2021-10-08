#include <iostream>
#include <base/scopeguard.h>
#include <base/str_utilts.h>
#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/asiodevicetype.h>
#include <metadata/win32/ntfssearch.h>
#include <player/audio_player.h>

using namespace xamp;
using namespace player;
using namespace base;
using namespace output_device;
using namespace win32;
using namespace xamp::metadata::win32;

void TestPlayDSD() {
	auto player = MakeAlignedShared<AudioPlayer>();
	player->Open("C:\\Users\\rdbill0452\\Music\\Test\\DSD.dsf", ExclusiveWasapiDeviceType::Id);
	player->PrepareToPlay();
	player->Play();
	std::cin.get();
	player->Stop();
}

void TestReadNTFSVolume() {
	auto file_record = MakeAlignedShared<NTFSFileRecord>();
	file_record->Open(L"\\\\\.\\C:");
	file_record->SetAttrMask(kNtfsAttrMaskIndexRoot | kNtfsAttrMaskIndexAllocation);
	file_record->ParseFileRecord(MFT_IDX_ROOT);
	file_record->ParseAttrs();
	file_record->TraverseSubEntries([&](auto entry) {
		});
}

int main() {	
	Logger::GetInstance()
		.AddDebugOutputLogger()
		.AddConsoleLogger()
		.AddFileLogger("xamp.log")
		.GetLogger("xamp");

	XAMP_SET_LOG_LEVEL(spdlog::level::debug);

	XAMP_ON_SCOPE_EXIT(
		Logger::GetInstance().Shutdown();
	);
	try
	{
		TestReadNTFSVolume();
	}catch (Exception const &e)
	{
		std::cout << e.GetErrorMessage();
		std::cin.get();
	}
	AudioPlayer::Initialize();
}
