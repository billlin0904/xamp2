#include <memory>
#include <sstream>
#include <iomanip>

#include "libebur128/ebur128.h"
#include <base/exception.h>
#include <base/audioformat.h>
#include <base/dataconverter.h>
#include <base/metadata.h>
#include <player/loudness_scanner.h>

namespace xamp::player {

static double Round(double d, int32_t index = 2) {
	std::stringstream ss;
	ss << std::fixed << std::setprecision(index) << d;
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
	explicit LoudnessScannerImpl(uint32_t sample_rate) {
		state_.reset(::ebur128_init(kMaxChannel, sample_rate, 
			EBUR128_MODE_I | EBUR128_MODE_TRUE_PEAK | EBUR128_MODE_SAMPLE_PEAK));
		IfFailThrow(::ebur128_set_channel(state_.get(), 0, EBUR128_LEFT));
		IfFailThrow(::ebur128_set_channel(state_.get(), 1, EBUR128_RIGHT));
	}

	void Process(float const* samples, size_t num_sample) {
		IfFailThrow(::ebur128_add_frames_float(state_.get(), samples, num_sample / kMaxChannel));
	}

	[[nodiscard]] double GetTruePeek() const {
		double left_true_peek = 0;
        double right_true_peek = 0;		
		IfFailThrow(::ebur128_true_peak(state_.get(), 0, &left_true_peek));
		IfFailThrow(::ebur128_true_peak(state_.get(), 1, &right_true_peek));
		return Round((left_true_peek + right_true_peek) / 2.0);
	}

    [[nodiscard]] double GetSamplePeak() const {
		double left_sample_peek = 0;
		double right_sample_peek = 0;
		IfFailThrow(::ebur128_sample_peak(state_.get(), 0, &left_sample_peek));
		IfFailThrow(::ebur128_sample_peak(state_.get(), 1, &right_sample_peek));
        return Round(std::max(left_sample_peek, right_sample_peek));
	}

	[[nodiscard]] double GetLoudness() const {
        double loudness = 0;
		IfFailThrow(::ebur128_loudness_global(state_.get(), &loudness));
        return Round(loudness);
	}

    void* GetHandle() const {
        return state_.get();
    }

    static double GetMultipleEbur128Gain(std::vector<AlignPtr<LoudnessScanner>> &scanners) {
        std::vector<ebur128_state*> handles;
        handles.reserve(scanners.size());
        for (auto const &scanner : scanners) {
            handles.push_back(static_cast<ebur128_state*>(scanner->GetNativeHandle()));
        }
        double loudness = 0;
        IfFailThrow(::ebur128_loudness_global_multiple(handles.data(), handles.size(), &loudness));
        return Round(loudness);
    }

private:
	struct Ebur128Deleter final {
		void operator()(ebur128_state* state) const noexcept {
			::ebur128_destroy(&state);
		}
	};

	using Ebur128StatePtr = std::unique_ptr<ebur128_state, Ebur128Deleter>;

	Ebur128StatePtr state_;	
};



LoudnessScanner::LoudnessScanner(uint32_t sample_rate)
    : impl_(MakeAlign<LoudnessScannerImpl>(sample_rate)) {
}

XAMP_PIMPL_IMPL(LoudnessScanner)

void LoudnessScanner::Process(float const * samples, size_t num_sample) {
	impl_->Process(samples, num_sample);
}

double LoudnessScanner::GetLoudness() const {
	return impl_->GetLoudness();
}

double LoudnessScanner::GetSamplePeak() const {
    return impl_->GetSamplePeak();
}

double LoudnessScanner::GetTruePeek() const {
	return impl_->GetTruePeek();
}

void* LoudnessScanner::GetNativeHandle() const {
    return impl_->GetHandle();
}

double LoudnessScanner::GetMultipleEbur128Gain(std::vector<AlignPtr<LoudnessScanner>> &scanners) {
    return LoudnessScannerImpl::GetMultipleEbur128Gain(scanners);
}

double LoudnessScanner::GetEbur128Gain(double loudness, double targetdb) {
	// EBUR128 sets the target level to -23 LUFS = 84dB
	// -23 - loudness = track gain to get to 84dB		
	return (-23.0 - loudness + targetdb - kReferenceLoudness);
}
	
}
