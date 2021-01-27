#include <cmath>
#include <limits>
#include <memory>
#include <sstream>
#include <iomanip>

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

static double Round(double d, int32_t index) {
	std::stringstream ss;
	ss << std::setprecision(index) << d << std::fixed;
	return std::stod(ss.str());
}

using Ebur128StatePtr = std::unique_ptr<ebur128_state, Ebur128Deleter>;

class LoudnessScanner::LoudnessScannerImpl {
public:
	LoudnessScannerImpl(uint32_t num_channels, uint32_t output_sample_rate)
		: num_channels_(num_channels)
		, output_sample_rate_(output_sample_rate) {
		Init();
	}
	
	void Init() {
		state_.reset(::ebur128_init(num_channels_, output_sample_rate_, EBUR128_MODE_I));		
		::ebur128_set_channel(state_.get(), 0, EBUR128_LEFT);
		::ebur128_set_channel(state_.get(), 1, EBUR128_RIGHT);
	}

	void Process(float const* samples, uint32_t num_sample) {
		::ebur128_add_frames_float(state_.get(), samples, num_sample / num_channels_);
	}

	[[nodiscard]] double GetLoudness() const {
		double loudness = 0;
		::ebur128_loudness_global(state_.get(), &loudness);
		return Round(loudness, 2);
	}
private:
	Ebur128StatePtr state_;
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
