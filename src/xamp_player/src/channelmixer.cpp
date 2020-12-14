#include <base/exception.h>
#include <player/channelmixer.h>

namespace xamp::player {

constexpr uint32_t kInitSampleSize = 8192 * 4;

ChannelMixer::ChannelMixer()
    : buffer_(kInitSampleSize) {
}

void ChannelMixer::Process(const float *samples, size_t num_sample, AudioBuffer<int8_t>& buffer) {
    if (buffer_.GetSize() < num_sample * 2) {
        buffer_.resize(num_sample * 2);
    }

    for (size_t i = 0; i < num_sample; ++i) {
        buffer_[i * 2] = samples[i];
        buffer_[i * 2 + 1] = samples[i];
    }

    CheckBufferFlow(buffer.TryWrite(reinterpret_cast<int8_t*>(buffer_.Get()), num_sample * 2));
}

}
