#include <player/spectrum.h>

namespace xamp::player {

//Spectrum::Spectrum(int32_t num_bands, int32_t min_freq, int32_t max_freq, int32_t frame_size, int32_t sample_rate) {
//	magnitude_.reserve(frame_size);
//}

void Spectrum::Init(size_t size) {
	magnitude_.resize(size);
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
