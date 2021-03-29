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
        : module_(LoadModule(GetDllFileName("libsoxr")))
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

	SoxrSampleRateConverterImpl() noexcept
		: enable_steep_filter_(false)
		, enable_dither_(false)
		, quality_(SoxrQuality::VHQ)
		, phase_(100.0)
		, input_sample_rate_(0)
		, num_channels_(0)
		, ratio_(0)
		, pass_band_(0.997)
		, stop_band_(1.0) {
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

		auto flags = (SOXR_ROLLOFF_NONE | SOXR_HI_PREC_CLOCK | SOXR_VR | SOXR_DOUBLE_PRECISION);
		if (enable_steep_filter_) {
			flags |= SOXR_STEEP_FILTER;
		}

		constexpr auto default_phase = SOXR_INTERMEDIATE_PHASE;
		auto soxr_quality = SoxrDLL.soxr_quality_spec(quality_spec | default_phase, flags);

		soxr_quality.passband_end = pass_band_;
		soxr_quality.stopband_begin = stop_band_;
		soxr_quality.phase_response = phase_;

		auto iospec = SoxrDLL.soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);

		if (!enable_dither_) {
			iospec.flags |= SOXR_NO_DITHER;
		}

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
			XAMP_LOG_DEBUG("soxr error: {}", !error ? "" : error);
			throw LibrarySpecException("sox_create return failure!");
		}

		input_sample_rate_ = input_sample_rate;
		output_sample_rate_ = output_sample_rate_;
		num_channels_ = num_channels;

		ratio_ = static_cast<double>(output_sample_rate) / static_cast<double>(input_sample_rate_);

		XAMP_LOG_DEBUG("Soxr resampler setting=> input:{} output:{} quality:{} phase:{} pass:{} stopband:{}",
			input_sample_rate,
			output_sample_rate,
			EnumToString(quality_),
			phase_,
			pass_band_,
			stop_band_);

		ResizeBuffer(kInitBufferSize);
	}

	[[nodiscard]] uint32_t GetOutPutSampleRate() const noexcept {
		return output_sample_rate_;
	}

	void Close() noexcept {
		handle_.reset();
		buffer_.reset();
	}

	void SetDither(bool enable) {
		enable_dither_ = enable;
	}

	void SetSteepFilter(bool enable) {
		enable_steep_filter_ = enable;
	}

	void SetQuality(SoxrQuality quality) {
		quality_ = quality;
	}

	void SetPhase(double phase) {
		phase_ = phase;
	}

	void SetPassBand(double passband) {
		pass_band_ = passband;
	}

	void SetStopBand(double stopband) {
		stop_band_ = stopband;
	}

	void Flush() {
		if (!handle_) {
			return;
		}
		SoxrDLL.soxr_clear(handle_.get());
	}

	bool Process(float const* samples, uint32_t num_sample, AudioBuffer<int8_t>& buffer) {
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

	bool Process(float const* samples, uint32_t num_sample, SampleWriter& writer) {
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
	bool enable_dither_;
	SoxrQuality quality_;
	double phase_;
	uint32_t input_sample_rate_;
	uint32_t output_sample_rate_;
	uint32_t num_channels_;
	double ratio_;
	double pass_band_;
	double stop_band_;
	SoxrHandle handle_;
	Buffer<float> buffer_;
};

const std::string_view SoxrSampleRateConverter::VERSION = "Soxr " SOXR_THIS_VERSION_STR;

SoxrSampleRateConverter::SoxrSampleRateConverter()
    : impl_(MakeAlign<SoxrSampleRateConverterImpl>()) {
}

XAMP_PIMPL_IMPL(SoxrSampleRateConverter)
	
void SoxrSampleRateConverter::LoadSoxrLib() {
    (void)Singleton<SoxrLib>::GetInstance();
}

void SoxrSampleRateConverter::Start(uint32_t input_sample_rate, uint32_t num_channels, uint32_t output_sample_rate) {
    impl_->Start(input_sample_rate, num_channels, output_sample_rate);
}

void SoxrSampleRateConverter::SetSteepFilter(bool enable) {
    impl_->SetSteepFilter(enable);
}

void SoxrSampleRateConverter::SetQuality(SoxrQuality quality) {
    impl_->SetQuality(quality);
}

void SoxrSampleRateConverter::SetPhase(double phase) {
    impl_->SetPhase(phase);
}

void SoxrSampleRateConverter::SetPassBand(double pass_band) {
    impl_->SetPassBand(pass_band);
}

void SoxrSampleRateConverter::SetStopBand(double stop_band) {
    impl_->SetStopBand(stop_band);
}

std::string_view SoxrSampleRateConverter::GetDescription() const noexcept {
    return VERSION;
}

bool SoxrSampleRateConverter::Process(float const * samples, uint32_t num_sample, AudioBuffer<int8_t>& buffer) {
    return impl_->Process(samples, num_sample, buffer);
}

bool SoxrSampleRateConverter::Process(float const* samples, uint32_t num_sample, SampleWriter& writer) {
    return impl_->Process(samples, num_sample, writer);
}

void SoxrSampleRateConverter::Flush() {
    impl_->Flush();
}

AlignPtr<SampleRateConverter> SoxrSampleRateConverter::Clone() {
    auto other = MakeAlign<SampleRateConverter, SoxrSampleRateConverter>();
    auto* converter = reinterpret_cast<SoxrSampleRateConverter*>(other.get());
    converter->SetQuality(impl_->quality_);
    converter->SetPassBand(impl_->pass_band_);
    converter->SetPhase(impl_->phase_);
    converter->SetStopBand(impl_->stop_band_);
    converter->SetSteepFilter(impl_->enable_steep_filter_);
    return other;
}

}

