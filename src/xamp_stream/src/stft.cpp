#include <base/memory.h>
#include <base/assert.h>
#include <base/audioformat.h>
#include <stream/stft.h>

XAMP_STREAM_NAMESPACE_BEGIN

STFT::STFT(size_t frame_size, size_t shift_size)
	: frame_size_(frame_size)
	, shift_size_(shift_size) {
    XAMP_EXPECTS(frame_size > 0);
    XAMP_EXPECTS(shift_size > 0);
    window_.Init(frame_size);
    fft_.Init(frame_size);
    output_size_ = frame_size - shift_size;
    buf_ = MakeBuffer<float>(frame_size);
    out_ = MakeBuffer<float>(frame_size);
    in_ = MakeBuffer<float>(frame_size);
}

void STFT::SetWindowType(WindowType type) {
    window_.SetWindowType(type);
    window_.Init(frame_size_);
}

const ComplexValarray& STFT::Process(const float* in, size_t length) {
    XAMP_EXPECTS(frame_size_ % AudioFormat::kMaxChannel == 0);
    XAMP_EXPECTS(frame_size_ >= length / AudioFormat::kMaxChannel);
    
    for (size_t i = 0; i < length / AudioFormat::kMaxChannel; ++i) {
        in_[i] = in[i * 2];
    }

    for (size_t i = 0; i < output_size_; ++i) {
        buf_[i] = buf_[i + shift_size_];
    }

    for (size_t i = 0; i < shift_size_; ++i) {
        buf_[output_size_ + i] = in_[i];
    }

    MemoryCopy(out_.data(),buf_.data(), sizeof(float) * frame_size_);
    window_(out_.data(), frame_size_);
    return fft_.Forward(out_.data(), frame_size_);
}

XAMP_STREAM_NAMESPACE_END