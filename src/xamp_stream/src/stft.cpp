#include <base/memory.h>
#include <base/assert.h>
#include <stream/stft.h>

namespace xamp::stream {

STFT::STFT(size_t channels, size_t frame_size, size_t shift_size)
	: channels_(channels)
	, frame_size_(frame_size)
	, shift_size_(shift_size) {
    window_.Init(frame_size);
    fft_.Init(frame_size);
    output_length_ = frame_size - shift_size;
    buf_.resize(frame_size);
    out_.resize(frame_size);
    in_.resize(frame_size);
}

const ComplexValarray& STFT::Process(const float* in, size_t length) {
    XAMP_ASSERT(frame_size_ % 2 == 0);
	XAMP_ASSERT(frame_size_ >= length / 2);
    
    for (size_t i = 0; i < length / 2; ++i) {
        in_[i] = in[i * 2];
    }

    for (size_t i = 0; i < output_length_; ++i) {
        buf_[i] = buf_[i + shift_size_];
    }

    for (size_t i = 0; i < shift_size_; ++i) {
        buf_[output_length_ + i] = in_[i];
    }

    MemoryCopy(out_.data(),buf_.data(), sizeof(float) * frame_size_);
    window_(out_.data(), frame_size_);
    return fft_.Forward(out_.data(), frame_size_);
}

}