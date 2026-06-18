#include <stream/bass_util.h>

#include <stream/bassfilestream.h>
#include <stream/basslib.h>

#include <base/buffer.h>

XAMP_STREAM_UTIL_NAMESPACE_BEGIN

uint32_t readStream(const BassStreamHandle& stream, float const* samples, float* out, size_t num_samples) {
    MemoryCopy(out, samples, num_samples * sizeof(float));
    const auto bytes_read =
        LIB_BASS.BASS_ChannelGetData(stream.get(),
            out,
            num_samples * sizeof(float));
    return bytes_read;
}

bool readStream(const BassStreamHandle& stream, float const* samples, size_t num_samples, BufferRef<float>& out) {
    if (out.size() != num_samples) {
        out.maybeResize(num_samples);
    }
    MemoryCopy(out.data(), samples, num_samples * sizeof(float));

    const auto bytes_read =
        LIB_BASS.BASS_ChannelGetData(stream.get(),
            out.data(),
            num_samples * sizeof(float));
    if (bytes_read == kBassError) {
        return false;
    }
    if (bytes_read == 0) {
        return false;
    }
    const auto frames = bytes_read / sizeof(float);
    out.maybeResize(frames);
    return true;
}

void encode(FileStream& stream, std::function<bool(uint32_t) > const& progress) {
    constexpr uint32_t kReadSampleSize = 8192 * 2;

    auto buffer = makeBuffer<float>(kReadSampleSize * AudioFormat::kMaxChannel);

    uint32_t num_samples = 0;
    const auto max_duration = static_cast<uint64_t>(stream.getDuration());

    while (stream.isActive()) {
        const auto read_size = stream.getSamples(buffer.data(), kReadSampleSize)
            / AudioFormat::kMaxChannel;
        if (read_size == kBassError || read_size == 0) {
            break;
        }
        num_samples += read_size;
        const auto percent = static_cast<uint32_t>(num_samples / stream.getFormat().getSampleRate() * 100 / max_duration);
        if (!progress(percent)) {
            break;
        }
    }
}

XAMP_STREAM_UTIL_NAMESPACE_END
