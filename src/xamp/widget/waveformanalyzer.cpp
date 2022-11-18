#include <widget/waveformanalyzer.h>

static int32_t computeTextureStride(int32_t size) {
    int32_t stride = 256;
    while (stride * stride < size) {
        stride *= 2;
    }
    return stride;
}

Waveform::Waveform(int32_t sample_rate,
    int32_t num_samples,
    int32_t desired_visual_sample_rate,
    int32_t max_visual_sample_rate) {
    texture_stride_ = 1024;
    num_visual_samples_ = 0;
    if (max_visual_sample_rate == -1) {
        if (desired_visual_sample_rate < sample_rate) {
            visual_sample_rate_ =
                static_cast<double>(desired_visual_sample_rate);
        }
        else {
            visual_sample_rate_ = static_cast<double>(sample_rate);
        }
    }
    else {
        if (sample_rate > max_visual_sample_rate) {
            visual_sample_rate_ = static_cast<double>(max_visual_sample_rate) *
                static_cast<double>(sample_rate) / static_cast<double>(sample_rate);
        }
        else {
            visual_sample_rate_ = sample_rate;
        }
    }
    visual_ratio_ = static_cast<double>(sample_rate) / visual_sample_rate_;
    num_visual_samples_ = static_cast<int32_t>(sample_rate / visual_ratio_) + 1;
    num_visual_samples_ += num_visual_samples_ % 2;
    Resize(num_visual_samples_, 0);
}

void Waveform::Resize(int32_t size, int32_t value) {
    data_size_ = size;
    texture_stride_ = computeTextureStride(size);
    data_.assign(texture_stride_ * texture_stride_, value);
}