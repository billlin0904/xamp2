#include <cmath>
#include <limits>
#include <memory>

#include "libebur128/ebur128.h"
#include <base/audioformat.h>
#include <base/dataconverter.h>
#include <player/loudness_scanner.h>

namespace xamp::player {

struct Ebur128Deleter {
	void operator()(ebur128_state *state) const {
		::ebur128_destroy(&state);
	}
};

using Ebur128StatePtr = std::unique_ptr<ebur128_state, Ebur128Deleter>;

class LoudnessScanner::LoudnessScannerImpl {
public:
	LoudnessScannerImpl(uint32_t num_channels, uint32_t output_sample_rate)
		: num_channels_(num_channels)
		, output_sample_rate_(output_sample_rate) {
		Init();
	}
	
	void Init() {
		ebur128_.reset(::ebur128_init(num_channels_, output_sample_rate_, EBUR128_MODE_I));		
		::ebur128_set_channel(ebur128_.get(), 0, EBUR128_LEFT);
		::ebur128_set_channel(ebur128_.get(), 1, EBUR128_RIGHT);
	}

	void Process(float const* samples, uint32_t num_sample) {
		::ebur128_add_frames_float(ebur128_.get(), samples, num_sample / num_channels_);
	}

	[[nodiscard]] double GetLoudness() const {
		double loudness = 0;
		::ebur128_loudness_global(ebur128_.get(), &loudness);
        loudness = std::round(loudness * 100) / 100.0;
		return loudness;
	}

	Ebur128StatePtr ebur128_;
	uint32_t num_channels_;
	uint32_t output_sample_rate_;
};

LoudnessScanner::LoudnessScanner(uint32_t num_channels, uint32_t output_sample_rate)
	: impl_(MakeAlign<LoudnessScannerImpl>(num_channels, output_sample_rate)) {
}

XAMP_PIMPL_IMPL(LoudnessScanner)

void LoudnessScanner::Process(float const * samples, uint32_t num_sample) {
	impl_->Process(samples, num_sample);
}

double LoudnessScanner::GetLoudness() const {
	return impl_->GetLoudness();
}
	
}
