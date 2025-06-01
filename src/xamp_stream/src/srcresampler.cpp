#include <stream/srcresampler.h>

#include <base/singleton.h>
#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>

#include <stream/srclib.h>

#include <stream/soxresampler.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(SrcSampleRateConverter);

class SrcSampleRateConverter::SrcSampleRateConverterImpl {
public:
	SrcSampleRateConverterImpl()
		: quality_(SrcQuality::SINC_HQ)
		, ratio_(0)
		, input_sample_rate_(0)
		, output_sample_rate_(0) {
		logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(SrcSampleRateConverter));
	}

	void Start(uint32_t output_sample_rate) {
		output_sample_rate_ = output_sample_rate;
	}

	void Initialize(uint32_t input_sample_rate) {
		input_sample_rate_ = input_sample_rate;

		int32_t error = 0;

		auto quality = 0;

		switch (quality_) {
		case SrcQuality::SINC_HQ:
			quality = SRC_SINC_BEST_QUALITY;
			break;
		case SrcQuality::SINC_MQ:
			quality = SRC_SINC_MEDIUM_QUALITY;
			break;
		case SrcQuality::SINC_LQ:
			quality = SRC_SINC_FASTEST;
			break;
		}

		handle_.reset(LIBSRC_LIB.src_new(quality, AudioFormat::kMaxChannel, &error));
		if (!handle_ || error > 0) {
			throw LibraryException(String::Format("src_new return failure! {}", LIBSRC_LIB.src_strerror(error)));
		}

		ratio_ = static_cast<double>(output_sample_rate_) / static_cast<double>(input_sample_rate_);
		if (!LIBSRC_LIB.src_is_valid_ratio(ratio_)) {
			throw LibraryException("Sample rate change out of valid range.");
		}

		XAMP_LOG_D(logger_, "quality: {}", quality_);
	}

	bool Process(float const* samples, size_t num_samples, BufferRef<float>& output) {
		const auto required_size = static_cast<size_t>(num_samples * ratio_);
		MaybeResizeBuffer(output, required_size);

		SRC_DATA src_data{};

		src_data.data_in = const_cast<SRC_SAMPLE*>(samples);
		src_data.input_frames = num_samples / AudioFormat::kMaxChannel;
		src_data.src_ratio = ratio_;

		size_t output_size = 0;

		src_data.data_out = output.data() + src_data.output_frames_gen;
		src_data.output_frames = output.size() / AudioFormat::kMaxChannel;

		const auto result = LIBSRC_LIB.src_process(handle_.get(), &src_data);
		if (result > 0) {
			return false;
		}

		if (src_data.output_frames_gen == 0) {
			return false;
		}

		if (src_data.end_of_input && src_data.output_frames_gen == 0) {
			return false;
		}

		src_data.data_in += src_data.input_frames_used * AudioFormat::kMaxChannel;
		src_data.input_frames -= src_data.input_frames_used;
		output_size += src_data.output_frames_gen;

		return true;
	}

	void SetQuality(SrcQuality quality) {
		quality_ = quality;
	}
private:
	void MaybeResizeBuffer(BufferRef<float>& output, size_t required_size) const {
		if (required_size > output.size()) {
			XAMP_LOG_D(logger_, "Resize size: {} => {}", output.size(), required_size);
		}
		output.maybe_resize(required_size);
	}

	struct SrcStateHandleTraits final {
		static SRC_STATE* invalid() noexcept {
			return nullptr;
		}

		static void close(SRC_STATE* value) noexcept {
			LIBSRC_LIB.src_delete(value);
		}
	};

	using SRCStateHandle = UniqueHandle<SRC_STATE*, SrcStateHandleTraits>;

	SrcQuality quality_;
	double ratio_;
	uint32_t input_sample_rate_;
	uint32_t output_sample_rate_;
	SRCStateHandle handle_;
	LoggerPtr logger_;
};

SrcSampleRateConverter::SrcSampleRateConverter()
	: impl_(MakeAlign<SrcSampleRateConverterImpl>()) {
}

XAMP_PIMPL_IMPL(SrcSampleRateConverter)

void SrcSampleRateConverter::SetQuality(SrcQuality quality) {
	return impl_->SetQuality(quality);
}

bool SrcSampleRateConverter::Process(float const* samples, size_t num_samples, BufferRef<float>& output) {
	return impl_->Process(samples, num_samples, output);
}

void SrcSampleRateConverter::Initialize(const AnyMap& config) {
	const auto output_format = config.Get<AudioFormat>(DspConfig::kOutputFormat);
	impl_->Start(output_format.GetSampleRate());

	const auto input_format = config.Get<AudioFormat>(DspConfig::kInputFormat);
	impl_->Initialize(input_format.GetSampleRate());
}

Uuid SrcSampleRateConverter::GetTypeId() const {
	return XAMP_UUID_OF(SrcSampleRateConverter);
}

std::string_view SrcSampleRateConverter::GetDescription() const noexcept {
	return "Secret Rabbit Code";
}

XAMP_STREAM_NAMESPACE_END