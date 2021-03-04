#include <player/spectrum.h>

namespace xamp::player {

Spectrum::Spectrum(int32_t num_bands, int32_t min_freq, int32_t max_freq, int32_t frame_size, int32_t sample_rate) {
	result_.reserve(frame_size);
}

void Spectrum::Process(std::valarray<Complex> const& frames) {
	result_.resize(frames.size());
	auto i = 0;
	for (auto const & frame : frames) {
		result_[i ++] = Mag2Db(frame);
	}
}

}
