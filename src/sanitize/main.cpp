#include <iostream>

#include <base/logger_impl.h>
#include <base/simd.h>
#include <base/rng.h>
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

void PrintArray(AlignArray<int8_t> const &input, size_t size) {
	for (auto i = 0; i < size; ++i) {
		std::cout << std::setw(2) << std::dec << std::setfill('0') <<
			static_cast<int32_t>(input[i]) << " ";
		if ((i + 1) % 8 == 0) std::cout << std::endl;
	}
}

void TestToInt8Planar() {
	auto input = MakeAlignedArray<int8_t>(64);
	auto value = 0;
	for (auto i = 0; i < 64; ++i) {
		input[i] = i % 2 == 0 ? ++value : 20 + value;
	}

	PrintArray(input, 64);
	std::cout << std::endl;

	auto left = MakeAlignedArray<int8_t>(64);
	auto right = MakeAlignedArray<int8_t>(64);

	InterleaveToPlanar<int8_t, int8_t>::Convert(
		input.get(),
		left.get(),
		right.get(),
		64);

	PrintArray(left, 64);
	std::cout << std::endl;

	PrintArray(right, 64);
}

int main(int argc, char* argv[]) {
	TestToInt8Planar();

	LoggerManager::GetInstance()
		.AddDebugOutput()
		.AddLogFile("xamp.log")
		.Startup();

	XampIniter initer;
	initer.Init();
}
