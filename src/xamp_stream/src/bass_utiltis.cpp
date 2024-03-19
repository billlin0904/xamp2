#include <stream/bass_utiltis.h>

#include <stream/bassfilestream.h>
#include <stream/basslib.h>

#include <base/buffer.h>

namespace xamp::stream::bass_utiltis {

uint32_t Process(BassStreamHandle& stream, float const* samples, float* out, uint32_t num_samples) {
    MemoryCopy(out, samples, num_samples * sizeof(float));
    const auto bytes_read =
        BASS_LIB.BASS_ChannelGetData(stream.get(),
            out,
            num_samples * sizeof(float));
    return bytes_read;
}

bool Process(BassStreamHandle& stream, float const * samples, uint32_t num_samples, BufferRef<float>& out) {
    if (out.size() != num_samples) {
        out.maybe_resize(num_samples);
    }
    MemoryCopy(out.data(), samples, num_samples * sizeof(float));

    const auto bytes_read =
            BASS_LIB.BASS_ChannelGetData(stream.get(),
                                 out.data(),
                                 num_samples * sizeof(float));
    if (bytes_read == kBassError) {
        return false;
    }
    if (bytes_read == 0) {
        return false;
    }
    const auto frames = bytes_read / sizeof(float);
    out.maybe_resize(frames);
    return true;
}

void Encode(FileStream& stream, std::function<bool(uint32_t) > const& progress) {
    constexpr uint32_t kReadSampleSize = 8192 * 2;

    auto buffer = MakeBuffer<float>(kReadSampleSize * AudioFormat::kMaxChannel);

    uint32_t num_samples = 0;
    const auto max_duration = static_cast<uint64_t>(stream.GetDurationAsSeconds());

    while (stream.IsActive()) {
        const auto read_size = stream.GetSamples(buffer.data(), kReadSampleSize)
            / AudioFormat::kMaxChannel;
        if (read_size == kBassError || read_size == 0) {
            break;
        }
        num_samples += read_size;
        const auto percent = static_cast<uint32_t>(num_samples / stream.GetFormat().GetSampleRate() * 100 / max_duration);
        if (!progress(percent)) {
            break;
        }
    }
}

}
