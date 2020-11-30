#include <base/dataconverter.h>
#include <stream/avfilestream.h>

#include <player/chromaprint.h>
#include <player/audio_player.h>
#include <player/audio_util.h>
#include <player/chromaprinthelper.h>

namespace xamp::player {

using namespace xamp::base;

std::tuple<double, std::vector<uint8_t>> ReadFingerprint(std::wstring const & file_path,
                                                         std::wstring const & file_ext,
                                                         std::function<bool(uint32_t)> &&progress) {
    constexpr uint32_t kFingerprintDuration = 120;
    constexpr uint32_t kReadSampleSize = 8192 * 4;

    auto file_stream = MakeFileStream(file_ext);
	file_stream->OpenFile(file_path);

	const auto source_format = file_stream->GetFormat();
	const AudioFormat input_format{
		DataFormat::FORMAT_PCM,
		source_format.GetChannels(),
		ByteFormat::FLOAT32,
		source_format.GetSampleRate(),
		PackedFormat::INTERLEAVED
	};

    auto isamples = MakeBufferPtr<float>(1024 + kReadSampleSize * input_format.GetChannels());
    auto osamples = MakeBufferPtr<int16_t>(1024 + kReadSampleSize * input_format.GetChannels());
    uint32_t num_samples = 0;

	const AudioFormat output_format { 
		DataFormat::FORMAT_PCM,
		input_format.GetChannels(),
		ByteFormat::FLOAT32,
		input_format.GetSampleRate(),
		PackedFormat::INTERLEAVED
	};

	const auto ctx = MakeConvert(input_format, input_format, kReadSampleSize);

	Chromaprint chromaprint;
	chromaprint.Start(input_format.GetSampleRate(), input_format.GetChannels(), kReadSampleSize);

	while (num_samples / input_format.GetSampleRate() < kFingerprintDuration) {
        auto deocode_size = file_stream->GetSamples(isamples.get(), kReadSampleSize) / input_format.GetChannels();
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

		DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>
            ::ConvertToInt16(osamples.get(), isamples.get(), ctx);
        chromaprint.Feed(osamples.get(), deocode_size * input_format.GetChannels());
	}	

	(void)chromaprint.Finish();

	return {
        file_stream->GetDuration(),
		chromaprint.GetFingerprint(),
	};
}

}
