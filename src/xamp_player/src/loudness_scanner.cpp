#include <memory>
#include <sstream>
#include <iomanip>

#include "libebur128/ebur128.h"
#include <base/exception.h>
#include <base/audioformat.h>
#include <base/dataconverter.h>
#include <player/loudness_scanner.h>

namespace xamp::player {

static double Round(double d, int32_t index) {
	std::stringstream ss;
	ss << std::setprecision(index) << d << std::fixed;
	return std::stod(ss.str());
}

#define IfFailThrow(expr) \
	do { \
		if ((expr) != EBUR128_SUCCESS) { \
			throw LibrarySpecException(#expr); \
		} \
	} while (false) 

class LoudnessScanner::LoudnessScannerImpl {
public:
	explicit LoudnessScannerImpl(uint32_t sample_rate)
        : sample_rate_(sample_rate) {
		Init();
	}
	
	void Init() {
        state_.reset(::ebur128_init(kMaxChannel, sample_rate_, EBUR128_MODE_I | EBUR128_MODE_TRUE_PEAK));
		IfFailThrow(::ebur128_set_channel(state_.get(), 0, EBUR128_LEFT));
		IfFailThrow(::ebur128_set_channel(state_.get(), 1, EBUR128_RIGHT));
	}

	void Process(float const* samples, uint32_t num_sample) {
		IfFailThrow(::ebur128_add_frames_float(state_.get(), samples, num_sample / kMaxChannel));
	}

	[[nodiscard]] double GetTruePeek() const {
		double left_true_peek = 0;
        double right_true_peek = 0;
		IfFailThrow(::ebur128_true_peak(state_.get(), 0, &left_true_peek));
		IfFailThrow(::ebur128_true_peak(state_.get(), 1, &right_true_peek));
		return Round((left_true_peek + right_true_peek) / 2.0, 2);
	}

	[[nodiscard]] double GetLoudness() const {
        double loudness = 0;
		IfFailThrow(::ebur128_loudness_global(state_.get(), &loudness));
        return Round(loudness, 2);
	}
private:
	struct Ebur128Deleter {
		void operator()(ebur128_state* state) const noexcept {
			::ebur128_destroy(&state);
		}
	};

	using Ebur128StatePtr = std::unique_ptr<ebur128_state, Ebur128Deleter>;

	uint32_t sample_rate_;
	Ebur128StatePtr state_;	
};

LoudnessScanner::LoudnessScanner(uint32_t sample_rate)
    : impl_(MakeAlign<LoudnessScannerImpl>(sample_rate)) {
}

XAMP_PIMPL_IMPL(LoudnessScanner)

void LoudnessScanner::Process(float const * samples, uint32_t num_sample) {
	impl_->Process(samples, num_sample);
}

double LoudnessScanner::GetLoudness() const {
	return impl_->GetLoudness();
}

double LoudnessScanner::GetTruePeek() const {
	return impl_->GetTruePeek();
}
	
}
