#include <vector>

#include <soxr.h>

#include <base/dll.h>
#include <base/logger.h>
#include <player/soxresampler.h>

namespace xamp::player {

class SoxrLib final {
public:
	SoxrLib() try
#ifdef _WIN32
		: module_(LoadDll("libsoxr.dll"))
#else
		: module_(LoadDll("libsoxr.dylib"))
#endif
		, soxr_quality_spec(module_, "soxr_quality_spec")
		, soxr_create(module_, "soxr_create")
		, soxr_process(module_, "soxr_process")
		, soxr_delete(module_, "soxr_delete") {
	}
	catch (const Exception & e) {
		XAMP_LOG_ERROR("{}", e.GetErrorMessage());
	}

	static XAMP_ALWAYS_INLINE SoxrLib& Instance() {
		static SoxrLib lib;
		return lib;
	}

	XAMP_DISABLE_COPY(SoxrLib)

private:
	ModuleHandle module_;

public:	
	XAMP_DEFINE_DLL_API(soxr_quality_spec) soxr_quality_spec;
	XAMP_DEFINE_DLL_API(soxr_create) soxr_create;
	XAMP_DEFINE_DLL_API(soxr_process) soxr_process;
	XAMP_DEFINE_DLL_API(soxr_delete) soxr_delete;
};

class SoxrResampler::SoxrResamplerImpl {
public:
	SoxrResamplerImpl()
		: allow_aliasing_(false)		
		, quality_(SoxrQuality::ULTIMATE)
		, phase_(SoxrPhase::LINEAR_PHASE)
		, ratio_(0)
		, handle_(nullptr) {
	}

	~SoxrResamplerImpl() {
		Close();
	}

	void Start(const AudioFormat& input_format, int32_t output_samplerate) {
		Close();

		long recipe = 0;

		switch (quality_) {
		case SoxrQuality::LOW:
			recipe |= SOXR_LQ;
			break;
		case SoxrQuality::QUICK:
			recipe |= SOXR_QQ;
			break;
		case SoxrQuality::MQ:
			recipe |= SOXR_MQ;
			break;
		case SoxrQuality::ULTIMATE:
			recipe |= SOXR_32_BITQ;
			break;
		}

		switch (phase_) {
		case SoxrPhase::LINEAR_PHASE:
			recipe |= SOXR_LINEAR_PHASE;
			break;
		case SoxrPhase::INTERMEDIATE_PHASE:
			recipe |= SOXR_INTERMEDIATE_PHASE;
			break;
		case SoxrPhase::MINIMUM_PHASE:
			recipe |= SOXR_MINIMUM_PHASE;
			break;
		}

		soxr_quality_spec_t q = SoxrLib::Instance().soxr_quality_spec(recipe, 0);
		soxr_error_t error = 0;
		handle_ = SoxrLib::Instance().soxr_create(input_format.GetSampleRate(),
			output_samplerate,
			input_format.GetChannels(),
			&error, 
			nullptr, 
			&q, 
			nullptr);
		ratio_ = (double)output_samplerate / input_format.GetSampleRate();
		input_format_ = input_format;
	}

	void Close() {
		if (handle_ != nullptr) {
			SoxrLib::Instance().soxr_delete(handle_);
			handle_ = nullptr;
		}
	}

	void SetAllowAliasing(bool allow) {
		allow_aliasing_ = allow;
	}

	void SetQuality(SoxrQuality quality) {
		quality_ = quality;
	}

	void SetPhase(SoxrPhase phase) {
		phase_ = phase;
	}

	void Process(const float* samples, int32_t num_sample, AudioBuffer<int8_t>& buffer) {
		buffer_.resize((size_t)(num_sample * ratio_) + 256);

		size_t samples_done = 0;

		auto error = SoxrLib::Instance().soxr_process(handle_, samples, num_sample / input_format_.GetChannels(),
			nullptr, buffer_.data(), buffer_.size() / input_format_.GetChannels(), &samples_done);

		buffer.TryWrite((int8_t*)buffer_.data(), buffer_.size() * sizeof(float));

		buffer_.resize(samples_done * input_format_.GetChannels());
	}

	bool allow_aliasing_;
	SoxrQuality quality_;
	SoxrPhase phase_;
	double ratio_;
	AudioFormat input_format_;
	soxr_t handle_;
	std::vector<float> buffer_;
};

SoxrResampler::SoxrResampler()
	: impl_(MakeAlign<SoxrResamplerImpl>()) {
}

SoxrResampler::~SoxrResampler() {
}

void SoxrResampler::Start(const AudioFormat& format, int32_t output_samplerate) {
	impl_->Start(format, output_samplerate);
}

void SoxrResampler::SetAllowAliasing(bool allow) {
	impl_->SetAllowAliasing(allow);
}

void SoxrResampler::SetQuality(SoxrQuality quality) {
	impl_->SetQuality(quality);
}

void SoxrResampler::SetPhase(SoxrPhase phase) {
	impl_->SetPhase(phase);
}

void SoxrResampler::Process(const float* samples, int32_t num_sample, AudioBuffer<int8_t>& buffer) {
	impl_->Process(samples, num_sample, buffer);
}

}
