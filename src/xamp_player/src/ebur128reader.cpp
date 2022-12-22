#include <memory>
#include <utility>

#include <ebur128/ebur128.h>
#include <base/singleton.h>
#include <base/exception.h>
#include <base/math.h>
#include <base/audioformat.h>
#include <base/dll.h>
#include <base/trackinfo.h>
#include <player/ebur128reader.h>

namespace xamp::player {

#define IfFailThrow(expr) \
	do { \
		if ((expr) != EBUR128_SUCCESS) { \
			throw LibrarySpecException(#expr); \
		} \
	} while (false)

static double Power2loudness(double power) {
	return 10 * log10(power) - 0.691;
}

static double Loudness2power(double loudness) {
	return pow(10, (loudness + 0.691) / 10.);
}

class Ebur128Lib final {
public:
	Ebur128Lib();

private:
    SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL(ebur128_init) ebur128_init;
	XAMP_DECLARE_DLL(ebur128_destroy) ebur128_destroy;
	XAMP_DECLARE_DLL(ebur128_set_channel) ebur128_set_channel;
	XAMP_DECLARE_DLL(ebur128_add_frames_float) ebur128_add_frames_float;
	XAMP_DECLARE_DLL(ebur128_true_peak) ebur128_true_peak;
	XAMP_DECLARE_DLL(ebur128_sample_peak) ebur128_sample_peak;
	XAMP_DECLARE_DLL(ebur128_loudness_global) ebur128_loudness_global;
	XAMP_DECLARE_DLL(ebur128_loudness_global_multiple) ebur128_loudness_global_multiple;
};

Ebur128Lib::Ebur128Lib()
	: module_(OpenSharedLibrary("ebur128"))
	, XAMP_LOAD_DLL_API(ebur128_init)
	, XAMP_LOAD_DLL_API(ebur128_destroy)
	, XAMP_LOAD_DLL_API(ebur128_set_channel)
	, XAMP_LOAD_DLL_API(ebur128_add_frames_float)
	, XAMP_LOAD_DLL_API(ebur128_true_peak)
	, XAMP_LOAD_DLL_API(ebur128_sample_peak)
	, XAMP_LOAD_DLL_API(ebur128_loudness_global)
	, XAMP_LOAD_DLL_API(ebur128_loudness_global_multiple) {
}

#define EBUR128_LIB Singleton<Ebur128Lib>::GetInstance()

class Ebur128Reader::Ebur128ReaderImpl {
public:
	Ebur128ReaderImpl() = default;

	void SetSampleRate(uint32_t sample_rate) {
		state_.reset(EBUR128_LIB.ebur128_init(AudioFormat::kMaxChannel, sample_rate,
			EBUR128_MODE_I | EBUR128_MODE_TRUE_PEAK | EBUR128_MODE_SAMPLE_PEAK));
		IfFailThrow(EBUR128_LIB.ebur128_set_channel(state_.get(), 0, EBUR128_LEFT));
		IfFailThrow(EBUR128_LIB.ebur128_set_channel(state_.get(), 1, EBUR128_RIGHT));
	}

	void Process(float const* samples, size_t num_sample) {
		IfFailThrow(EBUR128_LIB.ebur128_add_frames_float(state_.get(), 
			samples, num_sample / AudioFormat::kMaxChannel));
	}

	[[nodiscard]] double GetTruePeek() const {
		double left_true_peek = 0;
        double right_true_peek = 0;		
		IfFailThrow(EBUR128_LIB.ebur128_true_peak(state_.get(), 0, &left_true_peek));
		IfFailThrow(EBUR128_LIB.ebur128_true_peak(state_.get(), 1, &right_true_peek));
		return Round((left_true_peek + right_true_peek) / 2.0, 2);
	}

    [[nodiscard]] double GetSamplePeak() const {
		double left_sample_peek = 0;
		double right_sample_peek = 0;
		IfFailThrow(EBUR128_LIB.ebur128_sample_peak(state_.get(), 0, &left_sample_peek));
		IfFailThrow(EBUR128_LIB.ebur128_sample_peak(state_.get(), 1, &right_sample_peek));
        return Round((std::max)(left_sample_peek, right_sample_peek), 2);
	}

	[[nodiscard]] double GetLoudness() const {
        double loudness = 0;
		IfFailThrow(EBUR128_LIB.ebur128_loudness_global(state_.get(), &loudness));
        return Round(loudness, 2);
	}

    void* GetNativeHandle() const {
        return state_.get();
    }

    static double GetMultipleLoudness(const Vector<Ebur128Reader> &scanners) {
		Vector<ebur128_state*> handles;
        handles.reserve(scanners.size());
        for (auto const &scanner : scanners) {
            handles.push_back(static_cast<ebur128_state*>(scanner.GetNativeHandle()));
        }
        double loudness = 0;
        IfFailThrow(EBUR128_LIB.ebur128_loudness_global_multiple(handles.data(), handles.size(), &loudness));
        return Round(loudness, 2);
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
};

Ebur128Reader::Ebur128Reader()
    : impl_(MakePimpl<Ebur128ReaderImpl>()) {
}

void Ebur128Reader::SetSampleRate(uint32_t sample_rate) {
	impl_->SetSampleRate(sample_rate);
}

XAMP_PIMPL_IMPL(Ebur128Reader)

void Ebur128Reader::Process(float const * samples, size_t num_sample) {
	impl_->Process(samples, num_sample);
}

double Ebur128Reader::GetLoudness() const {
	return impl_->GetLoudness();
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

double Ebur128Reader::GetMultipleLoudness(const Vector<Ebur128Reader> &scanners) {
    return Ebur128ReaderImpl::GetMultipleLoudness(scanners);
}

double Ebur128Reader::GetEbur128Gain(double loudness, double targetdb) {
	// EBUR128 sets the target level to -23 LUFS = 84dB
	// -23 - loudness = track gain to get to 84dB		
	return (-23.0 - loudness + targetdb - kReferenceLoudness);
}

void Ebur128Reader::LoadEbur128Lib() {
	Singleton<Ebur128Lib>::GetInstance();
}
	
}
