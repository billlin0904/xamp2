#include <functional>
#include <utility>
#include <base/align_ptr.h>
#include <base/str_utilts.h>
#include <base/platform.h>
#include <base/dataconverter.h>

#include <stream/compressor.h>
#include <stream/wavefilewriter.h>
#include <stream/dsdstream.h>

#include <metadata/taglibmetawriter.h>

#include <player/chromaprint.h>
#include <player/audio_player.h>
#include <player/audio_util.h>
#include <player/loudness_scanner.h>
#include <player/samplerateconverter.h>

#include <widget/read_helper.h>

using namespace xamp::base;
using namespace xamp::stream;
using namespace xamp::metadata;

namespace Fs = std::filesystem;

inline constexpr uint64_t kFingerprintDuration = 120;
inline constexpr uint32_t kReadSampleSize = 8192 * 4;

static double ReadProcess(std::wstring const& file_path,
	std::wstring const& file_ext,
	std::function<bool(uint32_t)> const& progress,
	std::function<void(AudioFormat const&)> const& prepare,
	std::function<void(float const*, uint32_t)> const& func,
	uint64_t max_duration = std::numeric_limits<uint64_t>::max()) {
	const auto is_dsd_file = TestDsdFileFormatStd(file_path);
	auto file_stream = MakeStream(file_ext);

	if (auto* stream = dynamic_cast<DsdStream*>(file_stream.get())) {
		if (is_dsd_file) {
			stream->SetDSDMode(DsdModes::DSD_MODE_DSD2PCM);
		}
	}

	file_stream->OpenFile(file_path);

	const auto source_format = file_stream->GetFormat();
	const AudioFormat input_format = AudioFormat::ToFloatFormat(source_format);

	auto isamples = MakeBuffer<float>(
		1024 + kReadSampleSize * input_format.GetChannels());
	uint32_t num_samples = 0;

	prepare(input_format);

	if (max_duration == std::numeric_limits<uint64_t>::max()) {
		max_duration = static_cast<uint64_t>(file_stream->GetDuration());
	}

	int32_t retry = 0;
	while (num_samples / input_format.GetSampleRate() < max_duration) {
		const auto read_size = file_stream->GetSamples(isamples.get(),
			kReadSampleSize) / input_format.GetChannels();

		if (!read_size) {
			if (retry >= 3) {
				break;
			}
			++retry;
			continue;
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

class ExceptedFile {
public:	
	explicit  ExceptedFile(std::filesystem::path const& dest_file_path) {
		dest_file_path_ = dest_file_path;
		temp_file_path_ = Fs::temp_directory_path()
            / Fs::path(MakeTempFileName());
	}

	template <typename Func>
	void Try(Func&& func) {
		try {
			func(temp_file_path_);
			Fs::rename(temp_file_path_, dest_file_path_);
		}
		catch (...) {
			std::filesystem::remove(temp_file_path_);
		}
	}

private:
	Fs::path dest_file_path_;
	Fs::path temp_file_path_;
};

void Export2WaveFile(std::wstring const& file_path,
	std::wstring const& file_ext,
	std::wstring const& output_file_path,
	std::function<bool(uint32_t)> const& progress,
	Metadata const& metadata,
	uint32_t output_sample_rate,
	AlignPtr<SampleRateConverter>& converter) {
	ExceptedFile excepted(output_file_path);
	excepted.Try([&](auto const& dest_file_path)
		{
			WaveFileWriter file;
			Compressor compressor;
			auto prepare = [&file, &metadata, dest_file_path, &compressor, &converter, output_sample_rate](AudioFormat const& input_format) {
				auto format = input_format;
				format.SetByteFormat(ByteFormat::SINT24);
				format.SetSampleRate(output_sample_rate);
				file.Open(dest_file_path, format);
				compressor.SetSampleRate(input_format.GetSampleRate());
				compressor.Init();
				converter->Start(input_format.GetSampleRate(), input_format.GetChannels(), output_sample_rate);
			};

			auto process = [&file, &compressor, &converter](float const* samples, uint32_t sample_size) {
				auto const& buf = compressor.Process(samples, sample_size);
				converter->Process(buf.data(), buf.size(), file);
			};

			ReadProcess(file_path, file_ext, progress, prepare, process);
			file.Close();
			TaglibMetadataWriter writer;
			writer.Write(dest_file_path, metadata);
		});
}

void Export2WaveFile(std::wstring const& file_path,
	std::wstring const& file_ext,
	std::wstring const& output_file_path,
	std::function<bool(uint32_t)> const& progress,
	Metadata const& metadata,
	bool enable_compressor) {
	ExceptedFile excepted(output_file_path);
	excepted.Try([&](auto const& dest_file_path) {
		WaveFileWriter file;
		Compressor compressor;
		ReadProcess(file_path, file_ext, progress,
            [&file, dest_file_path, &compressor](AudioFormat const& input_format) {
				auto format = input_format;
				format.SetByteFormat(ByteFormat::SINT24);
				file.Open(dest_file_path, format);
				compressor.SetSampleRate(format.GetSampleRate());
				compressor.Init();
			}, [&file, &compressor, enable_compressor](auto const* samples, auto sample_size) {
				if (enable_compressor) {
					auto const& buf = compressor.Process(samples, sample_size);
					file.Write(buf.data(), buf.size());
				}
				else {
					file.Write(samples, sample_size);
				}
			});
		file.Close();
		TaglibMetadataWriter writer;
		writer.Write(dest_file_path, metadata);
		});
}

std::tuple<double, double> ReadFileLUFS(std::wstring const& file_path,
	std::wstring const& file_ext,
	std::function<bool(uint32_t)> const& progress) {
	std::optional<LoudnessScanner> scanner;

	ReadProcess(file_path, file_ext, progress,
		[&scanner](AudioFormat const& input_format)
		{
			scanner = LoudnessScanner(input_format.GetSampleRate());
		}, [&scanner](auto const* samples, auto sample_size)
		{
			scanner.value().Process(samples, sample_size);
		});

	return std::make_tuple(scanner->GetLoudness(), scanner->GetTruePeek());
}

std::tuple<double, std::vector<uint8_t>> ReadFingerprint(std::wstring const& file_path,
	std::wstring const& file_ext,
	std::function<bool(uint32_t)> const& progress) {
	Chromaprint chromaprint;

	std::vector<int16_t> osamples;
	AudioFormat convert_format;

	auto duration = ReadProcess(file_path, file_ext, progress,
        [&chromaprint, &convert_format](AudioFormat const& input_format)
		{
			convert_format = input_format;
			chromaprint.Start(input_format.GetSampleRate(), input_format.GetChannels());
		}, [&chromaprint, &convert_format, &osamples](auto const* samples, auto sample_size)
		{
			auto ctx = MakeConvert(convert_format, convert_format, sample_size / convert_format.GetChannels());
			osamples.resize(sample_size);
			DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>
				::ConvertToInt16(osamples.data(), samples, ctx);
			chromaprint.Feed(osamples.data(), sample_size);
		}, kFingerprintDuration);

	(void)chromaprint.Finish();

	return {
		duration,
		chromaprint.GetFingerprint(),
	};
}

