#include <widget/util/read_until.h>
#include <widget/util/str_utilts.h>
#include <widget/widget_shared.h>

#include <base/buffer.h>
#include <base/memory.h>
#include <base/logger_impl.h>
#include <base/exception.h>

#include <stream/idsdstream.h>
#include <stream/api.h>
#include <stream/filestream.h>
#include <stream/ifileencoder.h>

#include <metadata/api.h>
#include <metadata/imetadatawriter.h>

#include <stream/ebur128reader.h>

#include <functional>
#include <optional>
#include <utility>
#include <tuple>


namespace read_until {

inline constexpr uint32_t kReadSampleSize = 8192 * 4;

double readAll(Path const& file_path,
	std::function<bool(uint32_t)> const& progress,
	std::function<void(AudioFormat const&)> const& prepare,
	std::function<void(float const*, uint32_t)> const& dsp_process,
    uint64_t max_duration) {
	const auto is_dsd_file = IsDsdFile(file_path.wstring());
    const auto file_stream = StreamFactory::MakeFileStream(file_path,
		is_dsd_file
		? DsdModes::DSD_MODE_NATIVE : DsdModes::DSD_MODE_PCM);

	if (auto* stream = AsDsdStream(file_stream)) {
		if (is_dsd_file) {
			stream->SetDSDMode(DsdModes::DSD_MODE_DSD2PCM);
		}
	}

	file_stream->OpenFile(file_path.wstring());

	const auto source_format = file_stream->GetFormat();
	const AudioFormat input_format = AudioFormat::ToFloatFormat(source_format);

	const auto buffer_size = AlignUp(1024 + kReadSampleSize * input_format.GetChannels(), GetPageSize());
	auto buffer = MakeBuffer<float>(buffer_size);
	uint32_t num_samples = 0;

	prepare(input_format);

	if (max_duration == (std::numeric_limits<uint64_t>::max)()) {
		max_duration = static_cast<uint64_t>(file_stream->GetDurationAsSeconds());
	}

	uint32_t percent = 0;
    while (num_samples / input_format.GetSampleRate() < max_duration && file_stream->IsActive()) {
		const auto read_size = file_stream->GetSamples(buffer.get(),
			kReadSampleSize) / input_format.GetChannels();

		num_samples += read_size;
		if (progress != nullptr) {
			percent = static_cast<uint32_t>((num_samples / input_format.GetSampleRate() * 100) / max_duration);
			if (!progress(percent)) {
				break;
			}
		}

		dsp_process(buffer.get(), read_size * input_format.GetChannels());
	}

	return file_stream->GetDurationAsSeconds();
}

std::tuple<double, double> readFileLufs(Path const& file_path,
    std::function<bool(uint32_t)> const& progress,
    uint64_t max_duration) {
	Ebur128Reader reader;

    readAll(file_path, progress,
		[&reader](AudioFormat const& input_format)
		{
			reader.SetSampleRate(input_format.GetSampleRate());
		}, [&reader](auto const* samples, auto sample_size)
		{
			reader.Process(samples, sample_size);
        }, max_duration);

    return std::make_tuple(reader.GetIntegratedLoudness(),
                           reader.GetTruePeek());
}

void encodeFile(AnyMap const& config,
	AlignPtr<IFileEncoder>& encoder,
	std::function<bool(uint32_t)> const& progress,
	TrackInfo const& track_info) {
	const auto file_path = config.AsPath(FileEncoderConfig::kInputFilePath);
    const auto output_file_path = config.AsPath(FileEncoderConfig::kOutputFilePath);
    AnyMap copy_config = config;
	ExceptedFile excepted(output_file_path);
	if (excepted.Try([&](auto const& dest_file_path) {
		auto temp_path = QString::fromStdWString(dest_file_path.wstring());
        copy_config.AddOrReplace(FileEncoderConfig::kOutputFilePath, Path(toNativeSeparators(temp_path).toStdWString()));
		encoder->Start(copy_config);
		encoder->Encode(progress);
		encoder.reset();
		})) {
		const auto writer = MakeMetadataWriter();
		writer->Write(output_file_path, track_info);
	}
}

}
