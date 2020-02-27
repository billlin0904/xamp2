#include <vector>
#include <cassert>

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
		, soxr_delete(module_, "soxr_delete")
		, soxr_io_spec(module_, "soxr_io_spec")
		, soxr_runtime_spec(module_, "soxr_runtime_spec") {
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
	XAMP_DEFINE_DLL_API(soxr_io_spec) soxr_io_spec;
	XAMP_DEFINE_DLL_API(soxr_runtime_spec) soxr_runtime_spec;
};

class SoxrResampler::SoxrResamplerImpl {
public:
	SoxrResamplerImpl() noexcept
		: allow_aliasing_(false)		
		, quality_(SoxrQuality::VHQ)
		, phase_(SoxrPhase::LINEAR_PHASE)
		, input_samplerate_(0)
		, num_channels_(0)
		, ratio_(0)
		, handle_(nullptr) {
	}

	~SoxrResamplerImpl() {
		Close();
	}

	void Start(int32_t input_samplerate, int32_t num_channels, int32_t output_samplerate) {
		Close();

		long quality_spec = 0;

		switch (quality_) {
		case SoxrQuality::VHQ:
			quality_spec |= SOXR_32_BITQ;
			break;
		case SoxrQuality::HQ:
			quality_spec |= SOXR_HQ;
			break;
		case SoxrQuality::MQ:
			quality_spec |= SOXR_MQ;
			break;
		case SoxrQuality::LOW:
			quality_spec |= SOXR_LQ;
			break;		
		}

		auto soxr_quality = SoxrLib::Instance().soxr_quality_spec(quality_spec, (SOXR_HI_PREC_CLOCK | SOXR_VR));
		switch (phase_) {
		case SoxrPhase::LINEAR_PHASE:
			soxr_quality.phase_response = SOXR_LINEAR_PHASE;
			break;
		case SoxrPhase::INTERMEDIATE_PHASE:
			soxr_quality.phase_response = SOXR_INTERMEDIATE_PHASE;
			break;
		case SoxrPhase::MINIMUM_PHASE:
			soxr_quality.phase_response = SOXR_MINIMUM_PHASE;
			break;
		}

		auto iospec = SoxrLib::Instance().soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
		auto runtimespec = SoxrLib::Instance().soxr_runtime_spec(1);

		soxr_error_t error = 0;
		handle_ = SoxrLib::Instance().soxr_create(input_samplerate,
			output_samplerate,
			num_channels,
			&error, 
			&iospec,
			&soxr_quality,
			&runtimespec);

		input_samplerate_ = input_samplerate;
		num_channels_ = num_channels;

		ratio_ = (double)output_samplerate / input_samplerate_;
	}

	void Close() {
		if (handle_ != nullptr) {
			SoxrLib::Instance().soxr_delete(handle_);
			handle_ = nullptr;
		}
		buffer_.clear();
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

	bool Process(const float* samples, int32_t num_sample, AudioBuffer<int8_t>& buffer) {
		buffer_.resize((size_t)(num_sample * ratio_) + 256);

		size_t samples_done = 0;

		auto error = SoxrLib::Instance().soxr_process(handle_,
			samples,
			num_sample / num_channels_,
			nullptr,
			buffer_.data(),
			buffer_.size() / num_channels_,
			&samples_done);

		if (!samples_done) {
			return false;
		}

		buffer.TryWrite(reinterpret_cast<const int8_t*>(buffer_.data()),
			samples_done * num_channels_ * sizeof(float));
		buffer_.resize(samples_done * num_channels_);
		return true;
	}

	bool allow_aliasing_;
	SoxrQuality quality_;
	SoxrPhase phase_;
	int32_t input_samplerate_;
	int32_t num_channels_;
	double ratio_;
	soxr_t handle_;
	std::vector<float> buffer_;
};

SoxrResampler::SoxrResampler()
	: impl_(MakeAlign<SoxrResamplerImpl>()) {
}

SoxrResampler::~SoxrResampler() {
}

void SoxrResampler::LoadSoxrLib() {
	SoxrLib::Instance();
}

void SoxrResampler::Start(int32_t input_samplerate, int32_t num_channels, int32_t output_samplerate) {
	impl_->Start(input_samplerate, num_channels, output_samplerate);
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

bool SoxrResampler::Process(const float* samples, int32_t num_sample, AudioBuffer<int8_t>& buffer) {
	return impl_->Process(samples, num_sample, buffer);
}

}
