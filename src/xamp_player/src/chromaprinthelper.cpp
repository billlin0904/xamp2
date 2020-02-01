#include <player/chromaprint.h>
#include <stream/avfilestream.h>
#include <player/chromaprinthelper.h>

namespace xamp::player {

std::vector<uint8_t> ReadFingerprint(const std::wstring& file_path) {
	constexpr auto kFingerprintDuration = 120;
	constexpr auto kReadSampleSize = 8192;

	Chromaprint chromaprint;

	xamp::stream::AvFileStream stream;
	stream.OpenFromFile(file_path);

	const auto format = stream.GetFormat();
	chromaprint.Start(format.GetSampleRate(), format.GetChannels(), kReadSampleSize);

	std::vector<float> samples(kReadSampleSize * format.GetChannels());
	int32_t num_samples = 0;

	while (num_samples / format.GetSampleRate() < kFingerprintDuration) {
		num_samples += stream.GetSamples(samples.data(), kReadSampleSize);
		chromaprint.Feed(samples.data(), kReadSampleSize);
	}	

	chromaprint.Finish();

	return chromaprint.GetFingerprint();
}

}
