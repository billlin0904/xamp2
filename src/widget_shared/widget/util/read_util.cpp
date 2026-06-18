#include <QFile>
#include <QTextStream>
#include <widget/util/read_util.h>
#include <stream/filestream.h>

void readAll(Path const& file_path,
    std::function<bool(uint32_t)> const& progress,
    std::function<void(AudioFormat const&)> const& prepare,
    std::function<void(float const*, uint32_t)> const& dsp_process,
    uint64_t max_duration) {
    constexpr auto kReadSampleSize = 8192;

    const auto file_stream = makePcmFileStream(file_path);
    file_stream->openFile(file_path.wstring());

    const auto source_format = file_stream->getFormat();
    const AudioFormat input_format = AudioFormat::toFloatFormat(source_format);

    const auto buffer_size = 1024 + kReadSampleSize * input_format.getChannels();
    auto buffer = makeBuffer<float>(buffer_size);
    uint32_t num_samples = 0;

    prepare(input_format);

    if (max_duration == (std::numeric_limits<uint64_t>::max)()) {
        max_duration = static_cast<uint64_t>(file_stream->getDuration());
    }

    while (num_samples / input_format.getSampleRate() < max_duration && file_stream->isActive()) {
        const auto read_size = file_stream->getSamples(buffer.get(),
            kReadSampleSize) / input_format.getChannels();

        num_samples += read_size;
        if (progress != nullptr) {
            const auto percent = static_cast<uint32_t>((num_samples / input_format.getSampleRate() * 100) / max_duration);
            if (!progress(percent)) {
                break;
            }
        }

        dsp_process(buffer.get(), read_size * input_format.getChannels());
    }
}

ScopedPtr<FileStream> makePcmFileStream(const Path& file_path) {
    return StreamFactory::makeFileStream(file_path);
}

QString readAll(const QString& file_path) {
    QFile file_(file_path);
    if (!file_.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    QTextStream in(&file_);
    in.setEncoding(QStringConverter::Utf8);
	return in.readAll();
}
