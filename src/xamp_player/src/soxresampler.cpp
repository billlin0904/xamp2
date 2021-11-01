#include <cassert>

#include <soxr.h>

#include <base/singleton.h>
#include <base/dll.h>
#include <base/logger.h>
#include <base/buffer.h>
#include <player/soxresampler.h>

namespace xamp::player {

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
	static constexpr size_t kInitBufferSize = 1 * 1024 * 1204;
	static constexpr double kDefaultPassBand = 0.96;
	static constexpr double kDefaultStopBand = 1.0;
	static constexpr int32_t kDefaultPhase = 46;

	SoxrSampleRateConverterImpl() noexcept
		: enable_steep_filter_(false)
		, quality_(SoxrQuality::VHQ)
		, input_sample_rate_(0)
		, output_sample_rate_(0)
		, num_channels_(0)
        , rolloff_(SOXR_ROLLOFF_NONE)
		, ratio_(0)
		, pass_band_(kDefaultPassBand)
		, stop_band_(kDefaultStopBand)
		, phase_(kDefaultPhase) {
		logger_ = Logger::GetInstance().GetLogger(kResamplerLoggerName);
	}

	~SoxrSampleRateConverterImpl() noexcept {
		Close();
	}

	void Start(uint32_t input_sample_rate, uint32_t num_channels, uint32_t output_sample_rate) {
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

        auto flags = (rolloff_ | SOXR_HI_PREC_CLOCK | SOXR_VR | SOXR_DOUBLE_PRECISION | SOXR_NO_DITHER);
		if (enable_steep_filter_) {
			flags |= SOXR_STEEP_FILTER;
		}

		auto soxr_quality = SoxrDLL.soxr_quality_spec(quality_spec | phase_, flags);

		soxr_quality.passband_end = pass_band_ / 100.0;
		soxr_quality.stopband_begin = stop_band_ / 100.0;

		auto iospec = SoxrDLL.soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);

		auto runtimespec = SoxrDLL.soxr_runtime_spec(1);

		soxr_error_t error = nullptr;
		handle_.reset(SoxrDLL.soxr_create(input_sample_rate,
			output_sample_rate,
			num_channels,
			&error,
			&iospec,
			&soxr_quality,
			&runtimespec));
		if (!handle_) {
			XAMP_LOG_D(logger_, "soxr error: {}", !error ? "" : error);
			throw LibrarySpecException("sox_create return failure!");
		}

		input_sample_rate_ = input_sample_rate;
        output_sample_rate_ = output_sample_rate;
		num_channels_ = num_channels;

		ratio_ = static_cast<double>(output_sample_rate) / static_cast<double>(input_sample_rate_);

		XAMP_LOG_D(logger_, "Soxr resampler setting=> input:{} output:{} quality:{} phase:{} pass:{} stopband:{}",
			input_sample_rate,
			output_sample_rate,
			EnumToString(quality_),
			soxr_quality.phase_response,
			pass_band_,
			stop_band_);

		ResizeBuffer(kInitBufferSize);
	}

    void SetRollOffLevel(SoxrRollOff level) {
        switch (level) {
        case SoxrRollOff::ROLLOFF_SMALL:
            rolloff_ = SOXR_ROLLOFF_SMALL;
            break;
        case SoxrRollOff::ROLLOFF_MEDIUM:
            rolloff_ = SOXR_ROLLOFF_MEDIUM;
            break;
        case SoxrRollOff::ROLLOFF_NONE:
            rolloff_ = SOXR_ROLLOFF_NONE;
            break;
        }
    }

	void Close() noexcept {
		handle_.reset();
		buffer_.reset();
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
		if (phase >= 50) {
			phase_ = SOXR_LINEAR_PHASE;
		}
		if (phase < 50 && phase > 0) {
			phase_ = SOXR_INTERMEDIATE_PHASE;
		} else {
			phase_ = SOXR_MINIMUM_PHASE;
		}
	}

	void Flush() {
		if (!handle_) {
			return;
		}
		SoxrDLL.soxr_clear(handle_.get());
	}

	bool Process(float const* samples, size_t num_sample, AudioBuffer<int8_t>& buffer) {
		assert(num_channels_ != 0);

		auto required_size = static_cast<size_t>(num_sample * ratio_) + 256;
		if (required_size > buffer_.size()) {
			ResizeBuffer(required_size);
		}

		size_t samples_done = 0;
		SoxrDLL.soxr_process(handle_.get(),
			samples,
			num_sample / num_channels_,
			nullptr,
			buffer_.data(),
			buffer_.size() / num_channels_,
			&samples_done);

		if (!samples_done) {
			return false;
		}

		const auto write_size(samples_done * num_channels_ * sizeof(float));

		// 加入limiter之後就不再需進行ClampSample.
		// Note: libsoxr 並不會將sample進行限制大小(-1 < 0 < 1).
		//ClampSample(fifo_.data(), samples_done * num_channels_);

		BufferOverFlowThrow(buffer.TryWrite(reinterpret_cast<int8_t const*>(buffer_.data()), write_size));

		required_size = samples_done * num_channels_;
		if (required_size > buffer_.size()) {
			ResizeBuffer(required_size);
		}
		return true;
	}

	bool Process(float const* samples, size_t num_sample, SampleWriter& writer) {
		assert(num_channels_ != 0);

		auto required_size = static_cast<size_t>(num_sample * ratio_) + 256;
		if (required_size > buffer_.size()) {
			ResizeBuffer(required_size);
		}

		size_t samples_done = 0;
		SoxrDLL.soxr_process(handle_.get(),
			samples,
			num_sample / num_channels_,
			nullptr,
			buffer_.data(),
			required_size / num_channels_,
			&samples_done);

		if (!samples_done) {
			return false;
		}

		BufferOverFlowThrow(writer.TryWrite(buffer_.data(), samples_done * num_channels_));
		required_size = samples_done * num_channels_;
		if (required_size > buffer_.size()) {
			ResizeBuffer(required_size);
		}
		return true;
	}

	void ResizeBuffer(size_t required_size) {
		buffer_.resize(required_size);
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
    uint32_t rolloff_;
	double ratio_;
	double pass_band_;
	double stop_band_;
	int32_t phase_;
	SoxrHandle handle_;
	Buffer<float> buffer_;
	std::shared_ptr<spdlog::logger> logger_;
};

const std::string_view SoxrSampleRateConverter::VERSION = "Soxr " SOXR_THIS_VERSION_STR;

SoxrSampleRateConverter::SoxrSampleRateConverter()
    : impl_(MakeAlign<SoxrSampleRateConverterImpl>()) {
}

XAMP_PIMPL_IMPL(SoxrSampleRateConverter)

void SoxrSampleRateConverter::Start(uint32_t input_sample_rate, uint32_t num_channels, uint32_t output_sample_rate) {
    impl_->Start(input_sample_rate, num_channels, output_sample_rate);
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

void SoxrSampleRateConverter::SetRollOffLevel(SoxrRollOff level) {
    impl_->SetRollOffLevel(level);
}

std::string_view SoxrSampleRateConverter::GetDescription() const noexcept {
    return VERSION;
}

bool SoxrSampleRateConverter::Process(float const * samples, size_t num_sample, AudioBuffer<int8_t>& buffer) {
    return impl_->Process(samples, num_sample, buffer);
}

bool SoxrSampleRateConverter::Process(float const* samples, size_t num_sample, SampleWriter& writer) {
    return impl_->Process(samples, num_sample, writer);
}

bool SoxrSampleRateConverter::Process(Buffer<float> const& input, AudioBuffer<int8_t>& buffer) {
	return Process(input.data(), input.size(), buffer);
}

void SoxrSampleRateConverter::Flush() {
    impl_->Flush();
}

AlignPtr<ISampleRateConverter> SoxrSampleRateConverter::Clone() {
    auto other = MakeAlign<ISampleRateConverter, SoxrSampleRateConverter>();
    auto* converter = reinterpret_cast<SoxrSampleRateConverter*>(other.get());
    converter->SetQuality(impl_->quality_);
    converter->SetPassBand(impl_->pass_band_);
    converter->SetStopBand(impl_->stop_band_);
    converter->SetSteepFilter(impl_->enable_steep_filter_);
    return other;
}

void LoadSoxrLib() {
	(void)Singleton<SoxrLib>::GetInstance();
}
	
}

