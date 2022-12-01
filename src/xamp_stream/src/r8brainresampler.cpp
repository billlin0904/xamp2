#include <base/assert.h>
#include <base/logger_impl.h>
#include <stream/r8brainlib.h>
#include <stream/r8brainresampler.h>

namespace xamp::stream {

#define R8brainDLL Singleton<R8brainLib>::GetInstance()

const std::string_view R8brainSampleRateConverter::VERSION = "R8brain 5.6";

class R8brainSampleRateConverter::R8brainSampleRateConverterImpl {
public:
	R8brainSampleRateConverterImpl()
		: input_sample_rate_(0)
		, output_sample_rate_(0) {
		input_data_.resize(AudioFormat::kMaxChannel);
	}

	void Start(uint32_t output_sample_rate) {
		output_sample_rate_ = output_sample_rate;
	}

	void Init(uint32_t input_sample_rate) {
		handle_.reset(R8brainDLL.r8b_create(input_sample_rate,
			output_sample_rate_,
			kR8brainBufferSize,
			2.5,
			ER8BResamplerRes::r8brr24));
	}

	bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) {
		XAMP_ASSERT(num_samples <= kR8brainBufferSize);
		input_data_.resize(num_samples);

		for (auto i = 0; i < num_samples; ++i) {
			input_data_[i] = static_cast<double>(samples[i]);
		}

		double* outbuff = nullptr;
		auto read_count = R8brainDLL.r8b_process(handle_.get(),
			input_data_.data(),
			input_data_.size(),
			outbuff);

		XAMP_ASSERT(outbuff != nullptr);

		output.maybe_resize(read_count);

		for (auto i = 0; i < read_count; ++i) {
			output.data()[i] = static_cast<float>(outbuff[i]);
		}

		return true;
	}

	void Flush() {
	}

	struct CR8BResamplerHandleTraits final {
		static CR8BResampler invalid() noexcept {
			return nullptr;
		}

		static void close(CR8BResampler value) noexcept {
			R8brainDLL.r8b_clear(value);
			R8brainDLL.r8b_delete(value);
		}
	};

	using CR8BResamplerHandle = UniqueHandle<CR8BResampler, CR8BResamplerHandleTraits>;
	Vector<double> input_data_;
	uint32_t input_sample_rate_;
	uint32_t output_sample_rate_;
	CR8BResamplerHandle handle_;
};

R8brainSampleRateConverter::R8brainSampleRateConverter()
    : impl_(MakeAlign<R8brainSampleRateConverterImpl>()) {
}

XAMP_PIMPL_IMPL(R8brainSampleRateConverter)

bool R8brainSampleRateConverter::Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) {
	return impl_->Process(samples, num_samples, output);
}

void R8brainSampleRateConverter::Start(const DspConfig& config) {
	const auto output_format = std::any_cast<AudioFormat>(config.at(DspOptions::kOutputFormat));
	impl_->Start(output_format.GetSampleRate());
}

void R8brainSampleRateConverter::Init(const DspConfig& config) {
	const auto input_format = std::any_cast<AudioFormat>(config.at(DspOptions::kInputFormat));
	impl_->Init(input_format.GetSampleRate());
}

Uuid R8brainSampleRateConverter::GetTypeId() const {
	return Id;
}

void R8brainSampleRateConverter::Flush() {
    impl_->Flush();
}

std::string_view R8brainSampleRateConverter::GetDescription() const noexcept {
	return VERSION;
}


}