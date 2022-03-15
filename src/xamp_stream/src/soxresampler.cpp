#include <cassert>

#include <soxr.h>

#include <base/assert.h>
#include <base/singleton.h>
#include <base/dll.h>
#include <base/logger.h>
#include <base/buffer.h>
#include <stream/soxresampler.h>

namespace xamp::stream {

class SoxrLib final {
public:
    friend class Singleton<SoxrLib>;

    XAMP_DISABLE_COPY(SoxrLib)

private:
    SoxrLib() try
#ifdef XAMP_OS_WIN
        : module_(LoadModule("libsoxr.dll"))
#else
        : module_(LoadModule("libsoxr.dylib"))
#endif
        , soxr_quality_spec(module_, "soxr_quality_spec")
        , soxr_create(module_, "soxr_create")
        , soxr_process(module_, "soxr_process")
        , soxr_delete(module_, "soxr_delete")
        , soxr_io_spec(module_, "soxr_io_spec")
        , soxr_runtime_spec(module_, "soxr_runtime_spec")
        , soxr_clear(module_, "soxr_clear") {
		PrefetchModule(module_);
    }
    catch (const Exception& e) {
        XAMP_LOG_ERROR("{}", e.GetErrorMessage());
    }

    ModuleHandle module_;

public:
    XAMP_DECLARE_DLL(soxr_quality_spec) soxr_quality_spec;
    XAMP_DECLARE_DLL(soxr_create) soxr_create;
    XAMP_DECLARE_DLL(soxr_process) soxr_process;
    XAMP_DECLARE_DLL(soxr_delete) soxr_delete;
    XAMP_DECLARE_DLL(soxr_io_spec) soxr_io_spec;
    XAMP_DECLARE_DLL(soxr_runtime_spec) soxr_runtime_spec;
    XAMP_DECLARE_DLL(soxr_clear) soxr_clear;
};

#define SoxrDLL Singleton<SoxrLib>::GetInstance()

class SoxrSampleRateConverter::SoxrSampleRateConverterImpl final {
public:
	static constexpr double kDefaultPassBand = 0.96;
	static constexpr double kDefaultStopBand = 1.0;
	static constexpr int32_t kDefaultPhase = 46;

	SoxrSampleRateConverterImpl() noexcept
		: enable_steep_filter_(false)
		, quality_(SoxrQuality::VHQ)
		, input_sample_rate_(0)
		, output_sample_rate_(0)
		, num_channels_(0)
        , phase_(kDefaultPhase)
        , roll_off_(SoxrRollOff::ROLLOFF_NONE)
        , ratio_(0)
		, pass_band_(kDefaultPassBand)
        , stop_band_(kDefaultStopBand) {
		logger_ = Logger::GetInstance().GetLogger(kSoxrLoggerName);
	}

	~SoxrSampleRateConverterImpl() noexcept {
		Close();
	}

	void Init(uint32_t input_sample_rate) {
		Close();

		unsigned long quality_spec = 0;

		switch (quality_) {
		case SoxrQuality::UHQ:
			quality_spec |= SOXR_32_BITQ;
			break;
		case SoxrQuality::VHQ:
			quality_spec |= SOXR_VHQ;
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

		auto flags = (static_cast<int32_t>(roll_off_)
			| SOXR_HI_PREC_CLOCK
			| SOXR_VR
			| SOXR_DOUBLE_PRECISION
			| SOXR_NO_DITHER);
		if (enable_steep_filter_) {
			quality_spec |= SOXR_STEEP_FILTER;
		}

		auto phase = SOXR_LINEAR_PHASE;
		if (phase_ >= 50) {
			phase = SOXR_LINEAR_PHASE;
		}
		else if (phase_ < 50 && phase_ > 0) {
			phase = SOXR_INTERMEDIATE_PHASE;
		}
		else {
			phase = SOXR_MINIMUM_PHASE;
		}

		auto soxr_quality = SoxrDLL.soxr_quality_spec(quality_spec | phase, flags);

		soxr_quality.passband_end = pass_band_ / 100.0;
		soxr_quality.stopband_begin = stop_band_ / 100.0;

		auto iospec = SoxrDLL.soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);

		auto runtimespec = SoxrDLL.soxr_runtime_spec(1);

		soxr_error_t error = nullptr;
		handle_.reset(SoxrDLL.soxr_create(input_sample_rate,
			output_sample_rate_,
			kMaxChannel,
			&error,
			&iospec,
			&soxr_quality,
			&runtimespec));
		if (!handle_) {
			XAMP_LOG_D(logger_, "soxr error: {}", !error ? "" : error);
			throw LibrarySpecException("sox_create return failure!");
		}

		input_sample_rate_ = input_sample_rate;
		num_channels_ = kMaxChannel;

		ratio_ = static_cast<double>(output_sample_rate_) / static_cast<double>(input_sample_rate_);

		XAMP_LOG_D(logger_, "Soxr resampler setting=> input:{} output:{} quality:{} phase:{} passband:{} stopband:{} rolloff:{}",
			input_sample_rate_,
			output_sample_rate_,
			EnumToString(quality_),
			phase,
			pass_band_,
			stop_band_,
			EnumToString(roll_off_));
	}

	void Start(uint32_t output_sample_rate) {
		output_sample_rate_ = output_sample_rate;
	}

    void SetRollOff(SoxrRollOff level) {
		roll_off_ = level;
    }

	void Close() noexcept {
		handle_.reset();
	}

	void SetSteepFilter(bool enable) {
		enable_steep_filter_ = enable;
	}

	void SetQuality(SoxrQuality quality) {
		quality_ = quality;
	}

	void SetPassBand(double passband) {
		pass_band_ = passband;
	}

	void SetStopBand(double stopband) {
		stop_band_ = stopband;
	}

	void SetPhase(int32_t phase) {
		phase_ = phase;
	}

	void Flush() {
		if (!handle_) {
			return;
		}
		SoxrDLL.soxr_clear(handle_.get());
	}

	bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) {
		XAMP_ASSERT(num_channels_ != 0);

		auto required_size = static_cast<size_t>(num_samples * ratio_) + 256;
		MaybeResizeBuffer(output, required_size);

		size_t samples_done = 0;
		SoxrDLL.soxr_process(handle_.get(),
			samples,
			num_samples / num_channels_,
			nullptr,
			output.data(),
			output.size() / num_channels_,
			&samples_done);

		if (!samples_done) {
			return false;
		}

		if (samples_done * num_channels_ != output.size()) {
			output.maybe_resize(samples_done * num_channels_);
		}

		MemoryCopy(output.data(), output.data(), samples_done * num_channels_ * sizeof(float));
		required_size = samples_done * num_channels_;
		MaybeResizeBuffer(output, required_size);
		return true;
	}

	void MaybeResizeBuffer(BufferRef<float>& output, size_t required_size) {
		if (required_size > output.size()) {
			XAMP_LOG_D(logger_, "Resize size: {} => {}", output.size(), required_size);
		}
		output.maybe_resize(required_size);
	}

	struct SoxrHandleTraits final {
		static soxr_t invalid() noexcept {
			return nullptr;
		}

		static void close(soxr_t value) noexcept {
			SoxrDLL.soxr_delete(value);
		}
	};

	using SoxrHandle = UniqueHandle<soxr_t, SoxrHandleTraits>;

	bool enable_steep_filter_;	
	SoxrQuality quality_;
	uint32_t input_sample_rate_;
	uint32_t output_sample_rate_;
	uint32_t num_channels_;
	int32_t phase_;
	SoxrRollOff roll_off_;
	double ratio_;
	double pass_band_;
	double stop_band_;
	SoxrHandle handle_;
	std::shared_ptr<spdlog::logger> logger_;
};

const std::string_view SoxrSampleRateConverter::VERSION = "Soxr " SOXR_THIS_VERSION_STR;

SoxrSampleRateConverter::SoxrSampleRateConverter()
    : impl_(MakeAlign<SoxrSampleRateConverterImpl>()) {
}

XAMP_PIMPL_IMPL(SoxrSampleRateConverter)

void SoxrSampleRateConverter::Start(uint32_t output_sample_rate) {
	impl_->Start(output_sample_rate);
}

void SoxrSampleRateConverter::Init(uint32_t input_sample_rate) {
    impl_->Init(input_sample_rate);
}

void SoxrSampleRateConverter::SetSteepFilter(bool enable) {
    impl_->SetSteepFilter(enable);
}

void SoxrSampleRateConverter::SetQuality(SoxrQuality quality) {
    impl_->SetQuality(quality);
}

void SoxrSampleRateConverter::SetPassBand(double pass_band) {
    impl_->SetPassBand(pass_band);
}

void SoxrSampleRateConverter::SetStopBand(double stop_band) {
    impl_->SetStopBand(stop_band);
}

void SoxrSampleRateConverter::SetPhase(int32_t phase) {
	impl_->SetPhase(phase);
}

void SoxrSampleRateConverter::SetRollOff(SoxrRollOff level) {
    impl_->SetRollOff(level);
}

bool SoxrSampleRateConverter::Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) {
	return impl_->Process(samples, num_samples, output);
}

Uuid SoxrSampleRateConverter::GetTypeId() const {
	return Id;
}

void SoxrSampleRateConverter::Flush() {
    impl_->Flush();
}

std::string_view SoxrSampleRateConverter::GetDescription() const noexcept {
	return VERSION;
}

void LoadSoxrLib() {
	(void)Singleton<SoxrLib>::GetInstance();
}
	
}

