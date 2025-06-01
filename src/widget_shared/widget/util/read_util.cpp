#include <widget/util/read_util.h>
#include <stream/filestream.h>
#include <metadata/chromaprint.h>

void readAll(Path const& file_path,
    std::function<bool(uint32_t)> const& progress,
    std::function<void(AudioFormat const&)> const& prepare,
    std::function<void(float const*, uint32_t)> const& dsp_process,
    uint64_t max_duration) {
    constexpr auto kReadSampleSize = 8192;

    const auto file_stream = makePcmFileStream(file_path);
    file_stream->OpenFile(file_path.wstring());

    const auto source_format = file_stream->GetFormat();
    const AudioFormat input_format = AudioFormat::ToFloatFormat(source_format);

    const auto buffer_size = 1024 + kReadSampleSize * input_format.GetChannels();
    auto buffer = MakeBuffer<float>(buffer_size);
    uint32_t num_samples = 0;

    prepare(input_format);

    if (max_duration == (std::numeric_limits<uint64_t>::max)()) {
        max_duration = static_cast<uint64_t>(file_stream->GetDurationAsSeconds());
    }

    while (num_samples / input_format.GetSampleRate() < max_duration && file_stream->IsActive()) {
        const auto read_size = file_stream->GetSamples(buffer.get(),
            kReadSampleSize) / input_format.GetChannels();

        num_samples += read_size;
        if (progress != nullptr) {
            const auto percent = static_cast<uint32_t>((num_samples / input_format.GetSampleRate() * 100) / max_duration);
            if (!progress(percent)) {
                break;
            }
        }

        dsp_process(buffer.get(), read_size * input_format.GetChannels());
    }
}

Ebur128Scanner readFileLoudness(const Path& file_path, const std::function<bool(uint32_t)> & progress) {
    Ebur128Scanner scanner;
    auto prepare = [&scanner](AudioFormat const& input_format) {
        scanner.SetSampleRate(input_format.GetSampleRate());
        };
    auto dsp_process = [&scanner](auto const* samples, auto sample_size) {
        scanner.Process(samples, sample_size);
        };
    readAll(file_path, progress, prepare, dsp_process);
    return scanner;
}

QByteArray readChromaprint(const Path& file_path) {
    Chromaprint chromaprint;
    auto prepare = [&chromaprint](AudioFormat const& input_format) {
        chromaprint.SetSampleRate(input_format.GetSampleRate());
        };
    auto dsp_process = [&chromaprint](auto const* samples, auto sample_size) {
        chromaprint.Process(samples, sample_size);
        };
    auto progress = [](uint32_t) {
        return true;
        };
    readAll(file_path, progress, prepare, dsp_process);
    auto fingerprint = chromaprint.GetFingerprint();
    QByteArray base64(reinterpret_cast<const char*>(fingerprint.data()), fingerprint.size());
    return base64;
}

ScopedPtr<FileStream> makePcmFileStream(const Path& file_path) {
    PrefetchFile(file_path);
    auto file_stream = StreamFactory::MakeFileStream(file_path);
    file_stream->OpenFile(file_path);
    return file_stream;
}