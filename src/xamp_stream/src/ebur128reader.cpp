#include <memory>
#include <utility>

#include <ebur128/ebur128.h>
#include <base/singleton.h>
#include <base/exception.h>
#include <base/math.h>
#include <base/audioformat.h>
#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/trackinfo.h>
#include <stream/ebur128lib.h>
#include <stream/ebur128reader.h>

XAMP_STREAM_NAMESPACE_BEGIN

#define Ebur128IfFailThrow(expr) \
	do { \
		if ((expr) != EBUR128_SUCCESS) { \
			throw LibraryException(#expr); \
		} \
	} while (false)

namespace {

	float Power2Loudness(float power) {
		return 10 * log10(power) - 0.691;
	}

	float Loudness2Power(float loudness) {
		return pow(10, (loudness + 0.691) / 10.0);
	}

#define EBUR128_LIB Singleton<Ebur128Lib>::GetInstance()

	XAMP_DECLARE_LOG_NAME(Ebur128Reader);
}

class Ebur128Reader::Ebur128ReaderImpl {
public:
	Ebur128ReaderImpl() {
		logger_ = XampLoggerFactory.GetLogger(kEbur128ReaderLoggerName);
	}

	void SetSampleRate(uint32_t sample_rate) {
		int32_t lib_major{ 0 };
		int32_t lib_minor{ 0 };
		int32_t lib_patch{ 0 };
		EBUR128_LIB.ebur128_get_version(&lib_major, &lib_minor, &lib_patch);
		XAMP_LOG_D(logger_, "Library version: {}.{}.{}", lib_major, lib_minor, lib_patch);

		state_.reset(EBUR128_LIB.ebur128_init(AudioFormat::kMaxChannel, sample_rate,
			EBUR128_MODE_I | EBUR128_MODE_TRUE_PEAK | EBUR128_MODE_SAMPLE_PEAK));
		Ebur128IfFailThrow(EBUR128_LIB.ebur128_set_channel(state_.get(), 0, EBUR128_LEFT));
		Ebur128IfFailThrow(EBUR128_LIB.ebur128_set_channel(state_.get(), 1, EBUR128_RIGHT));
	}

	void Process(float const* samples, size_t num_sample) const {
		Ebur128IfFailThrow(EBUR128_LIB.ebur128_add_frames_float(state_.get(), 
			samples, num_sample / AudioFormat::kMaxChannel));
	}

	[[nodiscard]] double GetTruePeek() const {
		double left_true_peek = 0;
        double right_true_peek = 0;		
		Ebur128IfFailThrow(EBUR128_LIB.ebur128_true_peak(state_.get(), 0, &left_true_peek));
		Ebur128IfFailThrow(EBUR128_LIB.ebur128_true_peak(state_.get(), 1, &right_true_peek));
		const auto true_peak = (std::max)(left_true_peek, right_true_peek);
		return Round(true_peak, 6);
	}

    [[nodiscard]] double GetSamplePeak() const {
		double left_sample_peek = 0;
		double right_sample_peek = 0;
		Ebur128IfFailThrow(EBUR128_LIB.ebur128_sample_peak(state_.get(), 0, &left_sample_peek));
		Ebur128IfFailThrow(EBUR128_LIB.ebur128_sample_peak(state_.get(), 1, &right_sample_peek));
        return Round((std::max)(left_sample_peek, right_sample_peek), 2);
	}

	[[nodiscard]] double GetIntegratedLoudness() const {
        double loudness = 0;
		Ebur128IfFailThrow(EBUR128_LIB.ebur128_loudness_global(state_.get(), &loudness));
        return Round(loudness, 2);
	}

	[[nodiscard]] void* GetNativeHandle() const {
        return state_.get();
    }

    static double GetIntegratedMultipleLoudness(const Vector<Ebur128Reader> &scanners) {
		Vector<ebur128_state*> handles;
        handles.reserve(scanners.size());
        for (auto const &scanner : scanners) {
            handles.push_back(static_cast<ebur128_state*>(scanner.GetNativeHandle()));
        }
        double loudness = 0;
        Ebur128IfFailThrow(EBUR128_LIB.ebur128_loudness_global_multiple(handles.data(), handles.size(), &loudness));
        return Round(loudness, 6);
    }

private:
	struct Ebur128Deleter final {
		static ebur128_state* invalid() noexcept {
			return nullptr;
		}
		static void close(ebur128_state* value) {
			EBUR128_LIB.ebur128_destroy(&value);
		}
	};

	UniqueHandle<ebur128_state*, Ebur128Deleter> state_;
	LoggerPtr logger_;
};

Ebur128Reader::Ebur128Reader()
    : impl_(MakeAlign<Ebur128ReaderImpl>()) {
}

void Ebur128Reader::SetSampleRate(uint32_t sample_rate) {
	impl_->SetSampleRate(sample_rate);
}

XAMP_PIMPL_IMPL(Ebur128Reader)

void Ebur128Reader::Process(float const * samples, size_t num_sample) const {
	impl_->Process(samples, num_sample);
}

double Ebur128Reader::GetIntegratedLoudness() const {
	return impl_->GetIntegratedLoudness();
}

double Ebur128Reader::GetSamplePeak() const {
    return impl_->GetSamplePeak();
}

double Ebur128Reader::GetTruePeek() const {
	return impl_->GetTruePeek();
}

void* Ebur128Reader::GetNativeHandle() const {
    return impl_->GetNativeHandle();
}

double Ebur128Reader::GetIntegratedMultipleLoudness(const Vector<Ebur128Reader> &scanners) {
    return Ebur128ReaderImpl::GetIntegratedMultipleLoudness(scanners);
}

double Ebur128Reader::GetEbur128Gain(double lufs, double targetdb) {
	// EBUR128 sets the target level to -23 LUFS = 84dB
	// -23 - loudness = track gain to get to 84dB		
	return (kReferenceLoudness - lufs + targetdb);
}

XAMP_STREAM_NAMESPACE_END