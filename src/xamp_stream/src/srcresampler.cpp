#include <stream/srcresampler.h>

#include <base/singleton.h>
#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>

#include <stream/srclib.h>

XAMP_STREAM_NAMESPACE_BEGIN

#define LIBSRC_DLL Singleton<SrcLib>::GetInstance()

XAMP_DECLARE_LOG_NAME(SrcSampleRateConverter);

class SrcSampleRateConverter::SrcSampleRateConverterImpl {
public:
	SrcSampleRateConverterImpl()
		: ratio_(0)
		, input_sample_rate_(0)
		, output_sample_rate_(0) {
		logger_ = LoggerManager::GetInstance().GetLogger(kSrcSampleRateConverterLoggerName);
	}

	void Start(uint32_t output_sample_rate) {
		output_sample_rate_ = output_sample_rate;
	}

	void Init(uint32_t input_sample_rate) {
		input_sample_rate_ = input_sample_rate;
		int32_t error = 0;
		handle_.reset(LIBSRC_DLL.src_new(SRC_SINC_BEST_QUALITY, AudioFormat::kMaxChannel, &error));
		if (!handle_ || error > 0) {
			throw LibraryException(String::Format("src_new return failure! {}", LIBSRC_DLL.src_strerror(error)));
		}
		ratio_ = static_cast<double>(output_sample_rate_) / static_cast<double>(input_sample_rate_);
		if (!LIBSRC_DLL.src_is_valid_ratio(ratio_)) {
			throw LibraryException("Sample rate change out of valid range.");
		}
	}

	bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) {
		const auto required_size = static_cast<size_t>(num_samples * ratio_);
		MaybeResizeBuffer(output, required_size);

		SRC_DATA src_data{};

		src_data.data_in = samples;
		src_data.input_frames = num_samples / AudioFormat::kMaxChannel;
		src_data.src_ratio = ratio_;
		size_t output_size = 0;

		/*while (!src_data.end_of_input) {
			if (src_data.input_frames == 0) {
				return true;
			}

			if (src_data.input_frames < 8) {
				src_data.end_of_input = true;
			}

			src_data.data_out = output.data() + src_data.output_frames_gen;
			src_data.output_frames = output.size() / AudioFormat::kMaxChannel;

			const auto result = LIBSRC_DLL.src_process(handle_.get(), &src_data);
			if (result > 0) {
				return false;
			}

			if (src_data.output_frames_gen == 0) {
				return false;
			}

			if (src_data.end_of_input && src_data.output_frames_gen == 0) {
				break;
			}

			src_data.data_in += src_data.input_frames_used * AudioFormat::kMaxChannel;
			src_data.input_frames -= src_data.input_frames_used;
			output_size += src_data.output_frames_gen;
		}*/

		src_data.data_out = output.data() + src_data.output_frames_gen;
		src_data.output_frames = output.size() / AudioFormat::kMaxChannel;

		const auto result = LIBSRC_DLL.src_process(handle_.get(), &src_data);
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

	void Flush() {
	}

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
			LIBSRC_DLL.src_delete(value);
		}
	};

	using SRCStateHandle = UniqueHandle<SRC_STATE*, SrcStateHandleTraits>;

	double ratio_;
	uint32_t input_sample_rate_;
	uint32_t output_sample_rate_;
	SRCStateHandle handle_;
	LoggerPtr logger_;
};

SrcSampleRateConverter::SrcSampleRateConverter()
	: impl_(MakePimpl<SrcSampleRateConverterImpl>()) {
}

XAMP_PIMPL_IMPL(SrcSampleRateConverter)

bool SrcSampleRateConverter::Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) {
	return impl_->Process(samples, num_samples, output);
}

void SrcSampleRateConverter::Start(const AnyMap& config) {
	const auto output_format = config.Get<AudioFormat>(DspConfig::kOutputFormat);
	impl_->Start(output_format.GetSampleRate());
}

void SrcSampleRateConverter::Init(const AnyMap& config) {
	const auto input_format = config.Get<AudioFormat>(DspConfig::kInputFormat);
	impl_->Init(input_format.GetSampleRate());
}

Uuid SrcSampleRateConverter::GetTypeId() const {
	return XAMP_UUID_OF(SrcSampleRateConverter);
}

void SrcSampleRateConverter::Flush() {
	impl_->Flush();
}

std::string_view SrcSampleRateConverter::GetDescription() const noexcept {
	return "libsamplerate";
}

XAMP_STREAM_NAMESPACE_END