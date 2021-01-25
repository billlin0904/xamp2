#include <cmath>
#include <limits>
#include <memory>

#include "libebur128/ebur128.h"
#include <base/audioformat.h>
#include <base/dataconverter.h>
#include <player/replaygain.h>

namespace xamp::player {

// EBUR128 sets the target level to -23 LUFS = 84dB
// -> -23 - loudness = track gain to get to 84dB
static double Loudness2Scale(double lu, double reference_loudness = -18.0) {
	if (lu == std::numeric_limits<double>::quiet_NaN() 
		|| lu == std::numeric_limits<double>::infinity() || lu < -70) {
		return 1.0;
	} else {
		return std::pow(10.0, (reference_loudness - lu) / 20.0);
	}
}

struct Ebur128Deleter {
	void operator()(ebur128_state *state) const {
		::ebur128_destroy(&state);
	}
};

using Ebur128Ptr = std::unique_ptr<ebur128_state, Ebur128Deleter>;

class ReplayGain::ReplayGainImpl {
public:
	explicit ReplayGainImpl(uint32_t num_channels, uint32_t output_sample_rate)
		: num_channels_(num_channels)
		, output_sample_rate_(output_sample_rate)
		, total_num_sample_(0)
		, max_scale_(0) {
		Init();
	}
	
	void Init() {
		ebur128_.reset(::ebur128_init(num_channels_, output_sample_rate_, EBUR128_MODE_I));
		::ebur128_set_channel(ebur128_.get(), 0, EBUR128_LEFT);
		::ebur128_set_channel(ebur128_.get(), 1, EBUR128_RIGHT);
	}

	void Process(float * samples, uint32_t num_sample) {
		::ebur128_add_frames_float(ebur128_.get(), samples, num_sample);

		total_num_sample_ += num_sample;
		auto time_elapsed_ms = total_num_sample_ * 1000 / static_cast<double>(output_sample_rate_);;

		auto max_scale = 0.0;
				
		if (time_elapsed_ms < 400) {
			double in_loudness = 0;
			::ebur128_loudness_window(ebur128_.get(), time_elapsed_ms, &in_loudness);
			max_scale = in_loudness;
		} else {
			max_scale = GetLoudness();
		}

		/*max_scale_ = (std::max)(max_scale_, max_scale);

		auto scale = Loudness2Scale(max_scale_);
		for (uint32_t i = 0; i < num_sample * num_channels_; ++i) {
			samples[i] *= scale;
		}*/
	}

	[[nodiscard]] double GetLoudness() const {
		double loudness = 0;
		::ebur128_loudness_global(ebur128_.get(), &loudness);
		return loudness;
	}

	[[nodiscard]] double GetPeekPerChannel() const {
		double tr_peak = 0;
		for (auto ch = 0; ch < num_channels_; ++ch) {
			double ch_peak = 0;
			::ebur128_sample_peak(ebur128_.get(), ch, &ch_peak);
			tr_peak = (std::max)(ch_peak, tr_peak);
		}
		return tr_peak;
	}

	Ebur128Ptr ebur128_;
	uint32_t num_channels_;
	uint32_t output_sample_rate_;
	size_t total_num_sample_;
	double max_scale_;
};

ReplayGain::ReplayGain(uint32_t num_channels, uint32_t output_sample_rate)
	: impl_(MakeAlign<ReplayGainImpl>(num_channels, output_sample_rate)) {
}

XAMP_PIMPL_IMPL(ReplayGain)

void ReplayGain::Process(float* samples, uint32_t num_sample) {
	impl_->Process(samples, num_sample);
}
	
}
