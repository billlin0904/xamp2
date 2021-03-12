#include <utility>
#include <base/align_ptr.h>
#include <base/dataconverter.h>
#include <stream/dsdstream.h>

#include <stream/compressor.h>
#include <stream/wavefilewriter.h>

#include <base/str_utilts.h>
#include <player/chromaprint.h>
#include <player/audio_player.h>
#include <player/audio_util.h>
#include <player/loudness_scanner.h>
#include <player/samplerateconverter.h>
#include <player/read_helper.h>

namespace xamp::player {

using namespace xamp::base;

inline constexpr uint64_t kFingerprintDuration = 120;
inline constexpr uint32_t kReadSampleSize = 8192 * 4;

static double ReadProcess(std::wstring const& file_path,
	std::wstring const& file_ext,
	std::function<bool(uint32_t)> const& progress,	
	std::function<void(AudioFormat const &)> const& prepare,
    std::function<void(float const *, uint32_t)> const& func,
	uint64_t max_duration = std::numeric_limits<uint64_t>::max()) {
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

	auto isamples = MakeBuffer<float>(
		1024 + kReadSampleSize * input_format.GetChannels());	
	uint32_t num_samples = 0;

	prepare(input_format);

	if (max_duration == std::numeric_limits<uint64_t>::max()) {
		max_duration = static_cast<uint64_t>(file_stream->GetDuration());
	}

	while (num_samples / input_format.GetSampleRate() < max_duration) {
		const auto read_size = file_stream->GetSamples(isamples.get(), 
			kReadSampleSize) / input_format.GetChannels();

		if (!read_size) {
			break;
		}

		num_samples += read_size;
		if (progress != nullptr) {
			const auto percent = static_cast<uint32_t>((num_samples / input_format.GetSampleRate() * 100) / max_duration);
			if (!progress(percent)) {
				break;
			}
		}

        func(isamples.get(), read_size * input_format.GetChannels());
	}

	return file_stream->GetDuration();
}

void Export2WaveFile(std::wstring const& file_path,
	std::wstring const& file_ext,
	std::wstring const& output_file_path,
	std::function<bool(uint32_t)> const& progress,
	Metadata const& metadata,
	uint32_t output_sample_rate,
	AlignPtr<SampleRateConverter>& converter) {
	WaveFileWriter file;
	Compressor compressor;
	ReadProcess(file_path, file_ext, progress,
		[&file, &metadata, output_file_path, &compressor, &converter, output_sample_rate](AudioFormat const& input_format) {
			auto format = input_format;
			format.SetByteFormat(ByteFormat::SINT24);
			format.SetSampleRate(output_sample_rate);
			file.Open(output_file_path, format);
			compressor.SetSampleRate(input_format.GetSampleRate());
			compressor.Init();
			converter->Start(input_format.GetSampleRate(), input_format.GetChannels(), output_sample_rate);
		}, [&file, &compressor, &converter](float const* samples, uint32_t sample_size) {
			auto const& buf = compressor.Process(samples, sample_size);
			converter->Process(buf.data(), buf.size(), file);
		});
}

void Export2WaveFile(std::wstring const& file_path,
	std::wstring const& file_ext,
	std::wstring const& output_file_path,
	std::function<bool(uint32_t)> const& progress,
	Metadata const& metadata,
	bool enable_compressor) {
	WaveFileWriter file;
	Compressor compressor;	
	ReadProcess(file_path, file_ext, progress,
		[&file, &metadata, output_file_path, &compressor](AudioFormat const& input_format) {
			auto format = input_format;
			format.SetByteFormat(ByteFormat::SINT24);
			file.Open(output_file_path, format);
			compressor.SetSampleRate(format.GetSampleRate());
			compressor.Init();
		}, [&file, &compressor, enable_compressor](float const* samples, uint32_t sample_size) {
			if (enable_compressor) {
				auto const& buf = compressor.Process(samples, sample_size);
				file.Write(buf.data(), buf.size());
			} else {
				file.Write(samples, sample_size);
			}			
		});
	file.SetAlbum(String::ToString(metadata.album));
	file.SetTitle(String::ToString(metadata.title));
	file.SetArtist(String::ToString(metadata.artist));
	file.SetTrackNumber(metadata.track);
	file.Close();
}

std::tuple<double, double> ReadFileLUFS(std::wstring const& file_path, 
	std::wstring const& file_ext,
	std::function<bool(uint32_t)> const& progress) {
	std::optional<LoudnessScanner> scanner;
	
	ReadProcess(file_path, file_ext, progress,
		[&scanner](AudioFormat const& input_format)
		{
            scanner = LoudnessScanner(input_format.GetSampleRate());
		}, [&scanner](float const* samples, uint32_t sample_size)
		{
			scanner.value().Process(samples, sample_size);
		});

	return std::make_tuple(scanner->GetLoudness(), scanner->GetTruePeek());
}

std::tuple<double, std::vector<uint8_t>> ReadFingerprint(std::wstring const & file_path,
                                                         std::wstring const & file_ext,
                                                         std::function<bool(uint32_t)> const& progress) {
	Chromaprint chromaprint;

	auto duration = ReadProcess(file_path, file_ext, progress,
		[&chromaprint](AudioFormat const &input_format)
		{
			chromaprint.Start(input_format.GetSampleRate(), 
				input_format.GetChannels(),
				kReadSampleSize);
		}, [&chromaprint](float const * samples, uint32_t sample_size)
		{
			chromaprint.Feed(samples, sample_size * kMaxChannel);
		}, kFingerprintDuration);

	(void)chromaprint.Finish();

	return {
		duration,
		chromaprint.GetFingerprint(),
	};
}

}
