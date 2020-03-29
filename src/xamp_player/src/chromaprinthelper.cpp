#include <base/dataconverter.h>
#include <stream/avfilestream.h>

#include <player/chromaprint.h>
#include <player/audio_player.h>
#include <player/chromaprinthelper.h>

namespace xamp::player {

using namespace xamp::base;

Fingerprint ReadFingerprint(const std::wstring& file_path, const std::wstring& file_ext, std::function<bool(int32_t)> progress) {
	constexpr auto kFingerprintDuration = 120;
	constexpr auto kReadSampleSize = 8192 * 4;

	auto stream = AudioPlayer::MakeFileStream(file_ext);
	auto file_stream = dynamic_cast<FileStream*>(stream.get());
	file_stream->OpenFromFile(file_path);

	const auto source_format = file_stream->GetFormat();
	const AudioFormat input_format{
		DataFormat::FORMAT_PCM,
		source_format.GetChannels(),
		ByteFormat::FLOAT32,
		source_format.GetSampleRate(),
		InterleavedFormat::INTERLEAVED
	};

	std::vector<float> isamples(1024 + kReadSampleSize * input_format.GetChannels());
	std::vector<int16_t> osamples(1024 + kReadSampleSize * input_format.GetChannels());
	int32_t num_samples = 0;

	const AudioFormat output_format { 
		DataFormat::FORMAT_PCM,
		input_format.GetChannels(),
		ByteFormat::FLOAT32,
		input_format.GetSampleRate(),
		InterleavedFormat::INTERLEAVED
	};

	const auto ctx = MakeConvert(input_format, input_format, kReadSampleSize);

	Chromaprint chromaprint;
	chromaprint.Start(input_format.GetSampleRate(), input_format.GetChannels(), kReadSampleSize);

	while (num_samples / input_format.GetSampleRate() < kFingerprintDuration) {
		auto deocode_size = file_stream->GetSamples(isamples.data(), kReadSampleSize) / input_format.GetChannels();
		if (!deocode_size) {
			break;
		}

		num_samples += deocode_size;
		if (progress != nullptr) {
			auto percent = (num_samples / input_format.GetSampleRate() * 100) / kFingerprintDuration;
			if (!progress(percent)) {
				break;
			}
		}

		DataConverter<InterleavedFormat::INTERLEAVED, InterleavedFormat::INTERLEAVED>
			::ConvertToInt16(osamples.data(), isamples.data(), ctx);
		chromaprint.Feed(osamples.data(), deocode_size * input_format.GetChannels());
	}	

	chromaprint.Finish();

	return {
		stream->GetDuration(),
		chromaprint.GetFingerprint(),
	};
}

}
