#include <utility>
#include <base/align_ptr.h>
#include <base/dataconverter.h>
#include <stream/dsdstream.h>

#include <stream/compressor.h>
#include <player/chromaprint.h>
#include <player/audio_player.h>
#include <player/audio_util.h>
#include <player/loudness_scanner.h>
#include <player/read_helper.h>

namespace xamp::player {

using namespace xamp::base;

inline constexpr uint32_t kFingerprintDuration = 120;
inline constexpr uint32_t kReadSampleSize = 8192 * 4;

static double ReadProcess(std::wstring const& file_path,
	std::wstring const& file_ext,
	std::function<bool(uint32_t)> const& progress,
	uint32_t max_duration,
	std::function<void(AudioFormat const &, AudioFormat const&)> const& prepare,
    std::function<void(float const *, uint32_t)> const& func) {
    const auto is_dsd_file = audio_util::TestDsdFileFormatStd(file_path);
    auto file_stream = audio_util::MakeStream(file_ext);
	if (auto* stream = dynamic_cast<DsdStream*>(file_stream.get())) {
		if (is_dsd_file) {
			stream->SetDSDMode(DsdModes::DSD_MODE_DSD2PCM);
		}
	}
	file_stream->OpenFile(file_path);

	const auto source_format = file_stream->GetFormat();
	const AudioFormat input_format{
		DataFormat::FORMAT_PCM,
		source_format.GetChannels(),
		ByteFormat::FLOAT32,
		source_format.GetSampleRate(),
		PackedFormat::INTERLEAVED
	};

	auto isamples = MakeBufferPtr<float>(
		1024 + kReadSampleSize * input_format.GetChannels());	
	uint32_t num_samples = 0;

	const AudioFormat output_format{
		DataFormat::FORMAT_PCM,
		input_format.GetChannels(),
		ByteFormat::FLOAT32,
		input_format.GetSampleRate(),
		PackedFormat::INTERLEAVED
	};

	prepare(input_format, output_format);

	if (max_duration == std::numeric_limits<uint32_t>::max()) {
		max_duration = file_stream->GetDuration();
	}

	while (num_samples / input_format.GetSampleRate() < max_duration) {
		const auto read_size = file_stream->GetSamples(isamples.get(), 
			kReadSampleSize) / input_format.GetChannels();

		if (!read_size) {
			break;
		}

		num_samples += read_size;
		if (progress != nullptr) {
			const auto percent = (num_samples / input_format.GetSampleRate() * 100) / max_duration;
			if (!progress(percent)) {
				break;
			}
		}

        func(isamples.get(), read_size * input_format.GetChannels());
	}

	return file_stream->GetDuration();
}

std::tuple<double, double> ReadFileLUFS(std::wstring const& file_path, std::wstring const& file_ext, std::function<bool(uint32_t)> const& progress) {
	std::optional<LoudnessScanner> replay_gain;
	
	ReadProcess(file_path, file_ext, progress, std::numeric_limits<uint32_t>::max(),
        [&replay_gain](AudioFormat const& input_format, AudioFormat const&)
		{
            replay_gain = LoudnessScanner(input_format.GetSampleRate());
		}, [&replay_gain](float const* samples, uint32_t sample_size)
		{
			replay_gain.value().Process(samples, sample_size);
		});

	return std::make_tuple(replay_gain->GetLoudness(), replay_gain->GetTruePeek());
}

double GetGainScale(double lu, double reference_loudness) {
	// EBUR128 sets the target level to -23 LUFS = 84dB
	// -> -23 - loudness = track gain to get to 84dB

	if (lu == std::numeric_limits<double>::quiet_NaN()
		|| lu == std::numeric_limits<double>::infinity() || lu < -70) {
		return 1.0;
	}
	return std::pow(10.0, (reference_loudness - lu) / 20.0);
}

std::tuple<double, std::vector<uint8_t>> ReadFingerprint(std::wstring const & file_path,
                                                         std::wstring const & file_ext,
                                                         std::function<bool(uint32_t)> const& progress) {
	AudioConvertContext ctx;
	Chromaprint chromaprint;

	AlignBufferPtr<int16_t> osamples;

	auto duration = ReadProcess(file_path, file_ext, progress, kFingerprintDuration,
		[&ctx, &chromaprint, &osamples](AudioFormat const &input_format, AudioFormat const& output_format)
		{
			ctx = MakeConvert(input_format, output_format, kReadSampleSize);
			osamples = MakeBufferPtr<int16_t>(1024 + kReadSampleSize * input_format.GetChannels());
			chromaprint.Start(input_format.GetSampleRate(), 
				input_format.GetChannels(),
				kReadSampleSize);
		}, [&ctx, &chromaprint, &osamples](float const * samples, uint32_t sample_size)
		{
			DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>
				::ConvertToInt16(osamples.get(), samples, ctx);
			(void) chromaprint.Feed(osamples.get(), sample_size * kMaxChannel);
		});
	

	(void)chromaprint.Finish();

	return {
		duration,
		chromaprint.GetFingerprint(),
	};
}

}
