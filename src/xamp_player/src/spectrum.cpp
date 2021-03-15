#include <base/stl.h>
#include <base/logger.h>
#include <player/spectrum.h>

namespace xamp::player {

inline constexpr size_t kFFTSize = 32768;
inline constexpr size_t kVideoFrameRate = 30;
inline constexpr size_t kDisplaySize = 1024;
inline constexpr size_t kFreqVaryingOffset = static_cast<size_t>(kFFTSize * 0.000005);
inline constexpr size_t kFixedOffset = 0;

/*
https://dsp.stackexchange.com/questions/27703/how-does-adobe-after-effects-generate-its-audio-spectrum-effect

fft_length = 32768
display_length = 1024
freq_varying_offset = fft_length * 0.000005
for frame from 1 to ...:
    let start = frame / video_frame_rate * audio_sample_rate
    let samples = audio_data.subarray(start, start + fft_length)

    let buf = log_fft(window(samples), fft_length).subarray(0, display_length)
    let offset = fixed_offset - median(buf)
    for i from 0 to buf.length:
        buf[i] = buf[i] - offset + i * freq_varying_offset

 // now draw the bars on the video frame using the contents of buf
 */

void Spectrum::Init(AudioFormat const& format) {
	format_ = format;
	magnitude_.resize(kFFTSize);
	fft_.Init(kFFTSize);
	display_.resize(kDisplaySize);
	fifo_.Resize(16 * 1024 * 1024);
	buffer_.resize(kFFTSize);
	frame_size_ = 0;
	last_frame_size_ = 0;
}

const std::vector<float>& Spectrum::Update() {
	frame_size_++;

	const auto next_frame_size = frame_size_ / kVideoFrameRate * format_.GetSampleRate();

	if (last_frame_size_ != next_frame_size) {
		last_frame_size_ = next_frame_size;

		fifo_.TryRead(buffer_.data(), kFFTSize);

		Process(fft_.Forward(buffer_.data(), buffer_.size()));

		MemoryCopy(display_.data(), magnitude_.data(), display_.size() * sizeof(float));

		const auto mid = Median(display_);
		const auto offset = kFixedOffset - mid;
		
		for (size_t i = 0; i < display_.size(); ++i) {
			display_[i] = display_[i] - offset + i * kFreqVaryingOffset;
		}
	}
	return display_;
}
	
void Spectrum::Feed(float const* samples, size_t num_samples) {
	fifo_.TryWrite(samples, num_samples);
	//frame_size_++;
	//const auto next_frame_size = frame_size_ / kVideoFrameRate * format_.GetSampleRate();
	//XAMP_LOG_DEBUG("next_frame_size:{} ", next_frame_size);
}
	
float Spectrum::GetSpectralCentroid() const {
	auto sum_amplitudes = 0.0f;
	auto sum_weighted_amplitudes = 0.0f;
	auto i = 0;
	
	for (auto mag : magnitude_) {
		sum_amplitudes += mag;
		sum_weighted_amplitudes += mag * static_cast<float>(i++);
	}
	
	if (sum_amplitudes > 0) {
		return sum_weighted_amplitudes / sum_amplitudes;
	}
	return 0.0f;
}

void Spectrum::Process(std::valarray<Complex> const& frames) {
	magnitude_.resize(frames.size());
	auto i = 0;
	for (auto const & frame : frames) {
		magnitude_[i++] = std::sqrt(frame.real() * frame.real() + frame.imag() * frame.imag());
	}	
}

}
