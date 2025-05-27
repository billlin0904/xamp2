#include <ebur128/ebur128.h>

#include <base/singleton.h>
#include <base/exception.h>
#include <base/math.h>
#include <base/audioformat.h>
#include <base/dll.h>
#include <base/audioformat.h>

#include <stream/ebur128scanner.h>

XAMP_STREAM_NAMESPACE_BEGIN

#define IfFailThrow(expr) \
	do { \
		if ((expr) != EBUR128_SUCCESS) { \
			throw LibraryException(#expr); \
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
	XAMP_DECLARE_DLL_NAME(ebur128_init);
	XAMP_DECLARE_DLL_NAME(ebur128_destroy);
	XAMP_DECLARE_DLL_NAME(ebur128_set_channel);
	XAMP_DECLARE_DLL_NAME(ebur128_add_frames_float);
	XAMP_DECLARE_DLL_NAME(ebur128_true_peak);
	XAMP_DECLARE_DLL_NAME(ebur128_sample_peak);
	XAMP_DECLARE_DLL_NAME(ebur128_loudness_global);
	XAMP_DECLARE_DLL_NAME(ebur128_loudness_global_multiple);
};

Ebur128Lib::Ebur128Lib()
#ifdef XAMP_OS_WIN
	: module_(LoadSharedLibrary("ebur128.dll"))
#else
	: module_(LoadSharedLibrary("libebur128.dylib"))
#endif
	, XAMP_LOAD_DLL_API(ebur128_init)
	, XAMP_LOAD_DLL_API(ebur128_destroy)
	, XAMP_LOAD_DLL_API(ebur128_set_channel)
	, XAMP_LOAD_DLL_API(ebur128_add_frames_float)
	, XAMP_LOAD_DLL_API(ebur128_true_peak)
	, XAMP_LOAD_DLL_API(ebur128_sample_peak)
	, XAMP_LOAD_DLL_API(ebur128_loudness_global)
	, XAMP_LOAD_DLL_API(ebur128_loudness_global_multiple) {
}

#define EBUR128_LIB SharedSingleton<Ebur128Lib>::GetInstance()

constexpr auto kReferenceLoudness = -23.0;

class Ebur128Scanner::Ebur128ScannerImpl {
public:
	explicit Ebur128ScannerImpl(uint32_t sample_rate) {
		state_.reset(EBUR128_LIB.ebur128_init(AudioFormat::kMaxChannel,
			sample_rate,
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
		return kReferenceLoudness - Round((std::max)(left_sample_peek, right_sample_peek), 2);
	}

	[[nodiscard]] double GetLoudness() const {
		double loudness = 0;
		IfFailThrow(EBUR128_LIB.ebur128_loudness_global(state_.get(), &loudness));
		return kReferenceLoudness - Round(loudness, 2);
	}

	void* GetNativeHandle() const {
		return state_.get();
	}

	static double GetMultipleLoudness(std::vector<Ebur128Scanner>& scanners) {
		std::vector<ebur128_state*> handles;
		handles.reserve(scanners.size());
		for (auto const& scanner : scanners) {
			handles.push_back(static_cast<ebur128_state*>(scanner.GetNativeHandle()));
		}
		double loudness = 0;
		IfFailThrow(EBUR128_LIB.ebur128_loudness_global_multiple(handles.data(), handles.size(), &loudness));
		return kReferenceLoudness - Round(loudness, 2);
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

Ebur128Scanner::Ebur128Scanner(uint32_t sample_rate)
	: impl_(MakeAlign<Ebur128ScannerImpl>(sample_rate)) {
}

XAMP_PIMPL_IMPL(Ebur128Scanner)

void Ebur128Scanner::Process(float const* samples, size_t num_sample) {
	impl_->Process(samples, num_sample);
}

double Ebur128Scanner::GetLoudness() const {
	return impl_->GetLoudness();
}

double Ebur128Scanner::GetSamplePeak() const {
	return impl_->GetSamplePeak();
}

double Ebur128Scanner::GetTruePeek() const {
	return impl_->GetTruePeek();
}

void* Ebur128Scanner::GetNativeHandle() const {
	return impl_->GetNativeHandle();
}

double Ebur128Scanner::GetMultipleLoudness(std::vector<Ebur128Scanner>& scanners) {
	return Ebur128ScannerImpl::GetMultipleLoudness(scanners);
}

void Ebur128Scanner::LoadEbur128Lib() {
	EBUR128_LIB;
}

XAMP_STREAM_NAMESPACE_END
