#include <stream/soxresampler.h>

#include <stream/soxrlib.h>

#include <base/assert.h>
#include <base/singleton.h>
#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/buffer.h>

#include <soxr.h>

XAMP_STREAM_NAMESPACE_BEGIN

namespace {
	XAMP_DECLARE_LOG_NAME(SoxrSampleRateConverter);
	const std::string_view SOXR_Description = "Soxr " SOXR_THIS_VERSION_STR;
#define LISOXR_LIB SharedSingleton<SoxrLib>::GetInstance()	
}

class SoxrSampleRateConverter::SoxrSampleRateConverterImpl final {
public:
	static constexpr double kDefaultPassBand = 0.96;
	static constexpr double kDefaultStopBand = 1.0;
	static constexpr int32_t kDefaultPhase = 46;

	SoxrSampleRateConverterImpl() noexcept
		: enable_dither_(true)
		, quality_(SoxrQuality::SINC_VHQ)
		, input_sample_rate_(0)
		, output_sample_rate_(0)
		, num_channels_(0)
        , phase_(kDefaultPhase)
        , roll_off_(SoxrRollOff::ROLLOFF_NONE)
        , ratio_(0)
		, pass_band_(kDefaultPassBand)
        , stop_band_(kDefaultStopBand) {
		logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(SoxrSampleRateConverter));
	}

	~SoxrSampleRateConverterImpl() noexcept {
		Close();
	}

	void Init(uint32_t input_sample_rate) {
		Close();

		unsigned long quality_spec = 0;

		switch (quality_) {
		case SoxrQuality::SINC_UHQ:
			quality_spec |= SOXR_32_BITQ;
			break;
		case SoxrQuality::SINC_VHQ:
			quality_spec |= SOXR_VHQ;
			break;
		case SoxrQuality::SINC_HQ:
			quality_spec |= SOXR_HQ;
			break;
		case SoxrQuality::SINC_MQ:
			quality_spec |= SOXR_MQ;
			break;
		case SoxrQuality::SINC_LOW:
			quality_spec |= SOXR_LQ;
			break;
		}

		unsigned long flags = (static_cast<int32_t>(roll_off_)
			| SOXR_HI_PREC_CLOCK
			| SOXR_DOUBLE_PRECISION);

		// 因為昇頻可以降低系統量化雜訊的level，dithering可以降低系統的非線性因素，近而增加SNR。
		if (!enable_dither_) {
			flags |= SOXR_NO_DITHER;
			XAMP_LOG_D(logger_, "Soxr disable dither.");
		}

		unsigned long phase = SOXR_LINEAR_PHASE;
		if (phase_ >= 50) {
			phase = SOXR_LINEAR_PHASE;
		}
		else if (phase_ < 50 && phase_ > 0) {
			phase = SOXR_INTERMEDIATE_PHASE;
		}
		else {
			phase = SOXR_MINIMUM_PHASE;
		}

		auto soxr_quality = LISOXR_LIB.soxr_quality_spec(quality_spec | phase, flags);

		soxr_quality.passband_end = pass_band_ / 100.0;
		soxr_quality.stopband_begin = stop_band_ / 100.0;

		auto iospec = LISOXR_LIB.soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);

		auto runtimespec = LISOXR_LIB.soxr_runtime_spec(1);

		soxr_error_t error = nullptr;
		handle_.reset(LISOXR_LIB.soxr_create(input_sample_rate,
			output_sample_rate_,
			AudioFormat::kMaxChannel,
			&error,
			&iospec,
			&soxr_quality,
			&runtimespec));
		if (!handle_) {
			XAMP_LOG_D(logger_, "Soxr error: {}", !error ? "" : error);
			throw LibraryException("sox_create return failure!");
		}

		input_sample_rate_ = input_sample_rate;
		num_channels_ = AudioFormat::kMaxChannel;

		ratio_ = static_cast<double>(output_sample_rate_) / static_cast<double>(input_sample_rate_);

		XAMP_LOG_D(logger_, "Soxr resampler setting=> input:{} output:{} quality:{} phase:{} passband:{} stopband:{} rolloff:{} dither:{}.",
			input_sample_rate_,
			output_sample_rate_,
			EnumToString(quality_),
			phase,
			pass_band_,
			stop_band_,
			EnumToString(roll_off_),
			enable_dither_ ? "enable" : "disable");
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

	void SetDither(bool enable) {
		enable_dither_ = enable;
	}

	void Flush() {
		if (!handle_) {
			return;
		}
		LISOXR_LIB.soxr_clear(handle_.get());
	}

	bool Process(float const* samples, size_t num_samples, BufferRef<float>& output) {
		auto required_size = static_cast<size_t>(num_samples * ratio_) + 256;
		MaybeResizeBuffer(output, required_size);

		size_t num_read_samples = 0;
		LISOXR_LIB.soxr_process(handle_.get(),
			samples,
			num_samples / num_channels_,
			nullptr,
			output.data(),
			output.size() / num_channels_,
			&num_read_samples);

		if (!num_read_samples) {
			return false;
		}

		if (num_read_samples * num_channels_ != output.size()) {
			output.maybe_resize(num_read_samples * num_channels_);
		}

		MemoryCopy(output.data(), output.data(), num_read_samples * num_channels_ * sizeof(float));
		required_size = num_read_samples * num_channels_;
		MaybeResizeBuffer(output, required_size);
		return true;
	}

	void MaybeResizeBuffer(BufferRef<float>& output, size_t required_size) const {
		if (required_size > output.size()) {
			XAMP_LOG_D(logger_, "Resize size: {} => {}", output.size(), required_size);
		}
		output.maybe_resize(required_size);
	}

private:
	struct SoxrHandleTraits final {
		static soxr_t invalid() noexcept {
			return nullptr;
		}

		static void Close(soxr_t value) noexcept {
			LISOXR_LIB.soxr_delete(value);
		}
	};

	using SoxrHandle = UniqueHandle<soxr_t, SoxrHandleTraits>;
	bool enable_dither_;
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
	LoggerPtr logger_;
};

SoxrSampleRateConverter::SoxrSampleRateConverter()
    : impl_(MakeAlign<SoxrSampleRateConverterImpl>()) {
}

XAMP_PIMPL_IMPL(SoxrSampleRateConverter)

void SoxrSampleRateConverter::Initialize(const AnyMap& config) {
	const auto output_format = config.Get<AudioFormat>(DspConfig::kOutputFormat);
	impl_->Start(output_format.GetSampleRate());

	const auto input_format = config.Get<AudioFormat>(DspConfig::kInputFormat);
    impl_->Init(input_format.GetSampleRate());
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

void SoxrSampleRateConverter::SetDither(bool enable) {
	impl_->SetDither(enable);
}

bool SoxrSampleRateConverter::Process(float const* samples, size_t num_samples, BufferRef<float>& output) {
	return impl_->Process(samples, num_samples, output);
}

Uuid SoxrSampleRateConverter::GetTypeId() const {
	return XAMP_UUID_OF(SoxrSampleRateConverter);
}

std::string_view SoxrSampleRateConverter::GetDescription() const noexcept {
	return SOXR_Description;
}
	
XAMP_STREAM_NAMESPACE_END
