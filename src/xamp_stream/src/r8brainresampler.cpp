#include <stream/r8brainresampler.h>

#include <stream/r8brainlib.h>
#include <r8bbase.h>

#include <base/assert.h>
#include <base/logger.h>

XAMP_STREAM_NAMESPACE_BEGIN

namespace {	
	inline const std::string_view R8B_Description = "R8brain " R8B_VERSION;
}

class R8brainSampleRateConverter::R8brainSampleRateConverterImpl {
public:
	// The transition band is specified as the normalized spectral space
	// of the input signal.
	static constexpr double kReqTransBand = 2.5;

	R8brainSampleRateConverterImpl()
		: input_sample_rate_(0)
		, output_sample_rate_(0) {
		input_data_.resize(AudioFormat::kMaxChannel);
	}

	void start(uint32_t output_sample_rate) {
		output_sample_rate_ = output_sample_rate;
	}

	void initialize(uint32_t input_sample_rate) {
		handle_.reset(LIB_R8_DLL.r8b_create(input_sample_rate,
			output_sample_rate_,
			kR8brainBufferSize,
			kReqTransBand,
			ER8BResamplerRes::r8brr24));
	}

	bool process(float const* samples, size_t num_samples, BufferRef<float>& output) {
		XAMP_EXPECTS(num_samples <= kR8brainBufferSize);
		input_data_.resize(num_samples);

		for (auto i = 0; i < num_samples; ++i) {
			input_data_[i] = static_cast<double>(samples[i]);
		}

		double* outbuff = nullptr;
		const auto read_samples = LIB_R8_DLL.r8b_process(handle_.get(),
		                                               input_data_.data(),
		                                               static_cast<int32_t>(input_data_.size()),
		                                               outbuff);

		XAMP_ENSURES(outbuff != nullptr);

		output.maybeResize(read_samples);

		for (auto i = 0; i < read_samples; ++i) {
			output.data()[i] = static_cast<float>(outbuff[i]);
		}

		return true;
	}

	struct CR8BResamplerHandleTraits final {
		static CR8BResampler invalid() {
			return nullptr;
		}

		static void close(CR8BResampler value) {
			LIB_R8_DLL.r8b_clear(value);
			LIB_R8_DLL.r8b_delete(value);
		}
	};

	using CR8BResamplerHandle = UniqueHandle<CR8BResampler, CR8BResamplerHandleTraits>;
	std::vector<double> input_data_;
	uint32_t input_sample_rate_;
	uint32_t output_sample_rate_;
	CR8BResamplerHandle handle_;
};

R8brainSampleRateConverter::R8brainSampleRateConverter()
    : impl_(makeAlign<R8brainSampleRateConverterImpl>()) {
}

XAMP_PIMPL_IMPL(R8brainSampleRateConverter)

bool R8brainSampleRateConverter::process(float const* samples, size_t num_samples, BufferRef<float>& output) {
	return impl_->process(samples, num_samples, output);
}

void R8brainSampleRateConverter::initialize(const Property& config) {
	const auto output_format = config.get<AudioFormat>(DspConfig::kOutputFormat);
	impl_->start(output_format.getSampleRate());

	const auto input_format = config.get<AudioFormat>(DspConfig::kInputFormat);
	impl_->initialize(input_format.getSampleRate());
}

XAMP_STREAM_NAMESPACE_END
