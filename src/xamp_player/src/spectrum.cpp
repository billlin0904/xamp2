#include <player/spectrum.h>

namespace xamp::player {

inline constexpr size_t kFFTSize = 32768;
inline constexpr size_t kVideoFrameRate = 30;

//Spectrum::Spectrum(int32_t num_bands, int32_t min_freq, int32_t max_freq, int32_t frame_size, int32_t sample_rate) {
//	magnitude_.reserve(frame_size);
//}

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

static float GetMedian(std::vector<float> const &v) {
	auto n = v.size() / 2;
	std::nth_element(v.begin(), v.begin() + n, v.end());
	return v[n];
}

void Spectrum::Init(AudioFormat const& format) {
	format_ = format;
	magnitude_.resize(kFFTSize);
	fft_.Init(kFFTSize);
}

void Spectrum::Feed(float const* samples, size_t num_samples) {
	auto display_length = 1024;
	auto freq_varying_offset = kFFTSize * 0.000005;
	auto fixed_offset = 0;
	
	const auto start = num_samples / kVideoFrameRate * format_.GetSampleRate();
	Process(fft_.Forward(&samples[start], start + kFFTSize));
	
	auto offset = fixed_offset - GetMedian(magnitude_);
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
