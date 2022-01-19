#include <iostream>

#include <base/logger.h>
#include <base/simd.h>
#include <base/rng.h>
#include <base/dataconverter.h>
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

void TestToInterleave() {
	float left[8]{ 1, 1, 1, 1, 1, 1, 1, 1 };
	float right[8]{ 0, 0, 0, 0, 0, 0, 0, 0 };
	auto result = Converter<float>::ToInterleave(
		SIMD::LoadPs(left),
		SIMD::LoadPs(right));
}

void TestToF32ToI32Planar() {
	auto input = PRNG::GetInstance().GetRandomFloat(16, -1.0, 1.0);
	auto left = MakeAlignedArray<int32_t>(8);
	auto right = MakeAlignedArray<int32_t>(8);
	InterleaveToPlanar<float, int32_t>::Convert(
		input.get(),
		input.get() + 8,
		left.get(),
		right.get());
}

void TestFloatConverter() {
	const size_t kMaxTestSize = 4096;

	for (auto x = 0; x < 10000; ++x) {
		auto output = MakeAlignedArray<int32_t>(kMaxTestSize);
		auto input = PRNG::GetInstance().GetRandomFloat(kMaxTestSize, -1.0, 1.0);

		AudioFormat input_format;
		AudioFormat output_format;

		output_format.SetPackedFormat(PackedFormat::PLANAR);
		input_format.SetChannel(2);
		output_format.SetChannel(2);

		const auto ctx = MakeConvert(input_format, output_format, kMaxTestSize / 2);

		DataConverter<PackedFormat::INTERLEAVED,
			PackedFormat::PLANAR>::Convert(
				output.get(),
				input.get(),
				ctx);

		std::vector<int32_t> left(kMaxTestSize / 2);
		std::vector<int32_t> right(kMaxTestSize / 2);
		InterleaveToPlanar<float, int32_t>::Convert(
			input.get(),
			left.data(),
			right.data(),
			kMaxTestSize);

		std::vector<int32_t> combin;
		combin.insert(combin.end(), left.begin(), left.end());
		combin.insert(combin.end(), right.begin(), right.end());

		for (auto i = 0; i < kMaxTestSize; ++i) {
			if (combin[i] != output[i]) {
				std::cout << combin[i] << "!=" << output[i] << std::endl;
			}
		}
	}	
}

void TestToInt8Planar() {
	auto input = MakeAlignedArray<int8_t>(64);
	for (auto i = 0; i < 64; ++i) {
		input[i] = i % 2 == 0 ? 0 + i : 20 + i;
	}
	for (auto i = 0; i < 64; ++i) {
		std::cout << "0x" << std::setw(2) << std::dec << std::setfill('0') << 
			static_cast<int32_t>(input[i]) << " ";
		if ((i + 1) % 8 == 0) std::cout << std::endl;
	}
	std::cout << std::endl;
	auto left = MakeAlignedArray<int8_t>(32);
	auto right = MakeAlignedArray<int8_t>(32);
	InterleaveToPlanar<int8_t, int8_t>::Convert(
		input.get(),
		input.get() + 32,
		left.get(),
		right.get());
	for (auto i = 0; i < 32; ++i) {
		std::cout << "0x" << std::setw(2) << std::dec << std::setfill('0') <<
			static_cast<int32_t>(left[i]) << " ";
		if ((i + 1) % 8 == 0) std::cout << std::endl;
	}
	std::cout << std::endl;
	for (auto i = 0; i < 32; ++i) {
		std::cout << "0x" << std::setw(2) << std::dec << std::setfill('0') <<
			static_cast<int32_t>(right[i]) << " ";
		if ((i + 1) % 8 == 0) std::cout << std::endl;
	}
}

int main(int argc, char* argv[]) {
	//TestFloatConverter();
	//TestToInterleave();
	//TestToF32ToI32Planar();
	TestToInt8Planar();

	Logger::GetInstance()
		.AddDebugOutputLogger()
		.AddConsoleLogger()
		.AddFileLogger("xamp.log")
		.GetLogger(kDefaultLoggerName);

	XAMP_SET_LOG_LEVEL(spdlog::level::trace);

	XAMP_ON_SCOPE_EXIT(
		Logger::GetInstance().Shutdown();
	);

	XStartup();
	TestPlayDSD(String::ToStdWString(argv[1]));
}
