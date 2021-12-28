#include <functional>
#include <optional>
#include <utility>
#include <tuple>

#include <base/align_ptr.h>
#include <base/str_utilts.h>
#include <base/platform.h>
#include <base/dataconverter.h>

#include <stream/basscompressor.h>
#include <stream/wavefilewriter.h>
#include <stream/idsdstream.h>
#include <stream/api.h>
#include <stream/filestream.h>
#include <stream/ifileencoder.h>

#include <metadata/api.h>
#include <metadata/imetadatareader.h>
#include <metadata/imetadatawriter.h>

#include <player/loudness_scanner.h>
#include <stream/isamplerateconverter.h>

#include <widget/read_utiltis.h>

using namespace xamp::base;
using namespace xamp::stream;
using namespace xamp::player;
using namespace xamp::metadata;

namespace read_utiltis {

inline constexpr uint64_t kReadFingerprintDuration = 120;
inline constexpr uint32_t kReadSampleSize = 8192 * 4;

class ExceptedFile final {
public:
    explicit  ExceptedFile(std::filesystem::path const& dest_file_path) {
        dest_file_path_ = dest_file_path;
        temp_file_path_ = Fs::temp_directory_path()
                          / Fs::path(MakeTempFileName());
    }

    template <typename Func>
    bool Try(Func&& func) noexcept {
        try {
            func(temp_file_path_);
            Fs::rename(temp_file_path_, dest_file_path_);
			return true;
        }
        catch (...) {
            std::filesystem::remove(temp_file_path_);
        }
		return false;
    }

private:
    Fs::path dest_file_path_;
    Fs::path temp_file_path_;
};

double readAll(std::wstring const& file_path,
	std::function<bool(uint32_t)> const& progress,
	std::function<void(AudioFormat const&)> const& prepare,
	std::function<void(float const*, uint32_t)> const& dsp_process,
    uint64_t max_duration) {
	const auto is_dsd_file = TestDsdFileFormatStd(file_path);
    auto file_stream = MakeStream();

	if (auto* stream = AsDsdStream(file_stream)) {
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

    while (num_samples / input_format.GetSampleRate() < max_duration && file_stream->IsActive()) {
		const auto read_size = file_stream->GetSamples(isamples.get(),
			kReadSampleSize) / input_format.GetChannels();

		num_samples += read_size;
		if (progress != nullptr) {
			const auto percent = static_cast<uint32_t>((num_samples / input_format.GetSampleRate() * 100) / max_duration);
			if (!progress(percent)) {
				break;
			}
		}

		dsp_process(isamples.get(), read_size * input_format.GetChannels());
	}

	return file_stream->GetDuration();
}

void export2WaveFile(std::wstring const& file_path,
	std::wstring const& output_file_path,
	std::function<bool(uint32_t)> const& progress,
	Metadata const& metadata,
	uint32_t output_sample_rate,
	AlignPtr<ISampleRateConverter>& converter) {
	ExceptedFile excepted(output_file_path);
	excepted.Try([&](auto const& dest_file_path)
		{
			WaveFileWriter file;
            auto compressor = MakeCompressor();
            auto prepare = [&file, dest_file_path, &compressor, &converter, output_sample_rate](AudioFormat const& input_format) {
				auto format = input_format;
				format.SetByteFormat(ByteFormat::SINT24);
				format.SetSampleRate(output_sample_rate);
				file.Open(dest_file_path, format);
                compressor->Start(input_format.GetSampleRate());
                dynamic_cast<BassCompressor*>(compressor.get())->Init();
				converter->Start(input_format.GetSampleRate(), input_format.GetChannels(), output_sample_rate);
			};

            Buffer<float> buffer;
            auto process = [&file, &compressor, &converter, &buffer](float const* samples, uint32_t sample_size) {
                compressor->Process(samples, sample_size, buffer);
                return converter->Process(buffer.data(), buffer.size(), file);
			};

            readAll(file_path, progress, prepare, process);
			file.Close();
			auto writer = MakeMetadataWriter();
			writer->Write(dest_file_path, metadata);
		});
}

void export2WaveFile(std::wstring const& file_path,
	std::wstring const& output_file_path,
	std::function<bool(uint32_t)> const& progress,
	Metadata const& metadata,
	bool enable_compressor) {
	ExceptedFile excepted(output_file_path);
	excepted.Try([&](auto const& dest_file_path) {
		WaveFileWriter file;
        auto compressor = MakeCompressor();
        Buffer<float> buffer;
        readAll(file_path, progress,
            [&file, dest_file_path, &compressor](AudioFormat const& input_format) {
				auto format = input_format;
				format.SetByteFormat(ByteFormat::SINT24);
				file.Open(dest_file_path, format);
                compressor->Start(format.GetSampleRate());
                dynamic_cast<BassCompressor*>(compressor.get())->Init();
            }, [&file, &compressor, enable_compressor, &buffer](auto const* samples, auto sample_size) {
				if (enable_compressor) {
                    compressor->Process(samples, sample_size, buffer);
                    file.Write(buffer.data(), buffer.size());
				}
				else {
					file.Write(samples, sample_size);
				}
			});
		file.Close();
		auto writer = MakeMetadataWriter();
		writer->Write(dest_file_path, metadata);
		});
}

std::tuple<double, double> readFileLUFS(std::wstring const& file_path,
    std::function<bool(uint32_t)> const& progress,
    uint64_t max_duration) {
	std::optional<LoudnessScanner> scanner;

    readAll(file_path, progress,
		[&scanner](AudioFormat const& input_format)
		{
			scanner = LoudnessScanner(input_format.GetSampleRate());
		}, [&scanner](auto const* samples, auto sample_size)
		{
			scanner.value().Process(samples, sample_size);
        }, max_duration);

    return std::make_tuple(scanner->GetLoudness(),
                           scanner->GetTruePeek());
}

void encodeFlacFile(std::wstring const& file_path,
                std::wstring const& output_file_path,
                std::wstring const& command,
                std::function<bool(uint32_t)> const& progress,
                Metadata const& metadata) {
    ExceptedFile excepted(output_file_path);
    if (excepted.Try([&](auto const& dest_file_path) {
	    auto encoder = MakeFlacEncoder();
        encoder->Start(file_path, dest_file_path.wstring(), command);
        encoder->Encode(progress);
    })) {
		auto writer = MakeMetadataWriter();
		writer->Write(output_file_path, metadata);
    }
}

}
