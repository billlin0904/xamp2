#include <array>

#include <base/dll.h>
#include <base/logger.h>
#include <r8bsrc.h>
#include <player/cdspresampler.h>

namespace xamp::player {

class CdspLib final {
public:
	CdspLib() try
		: module_(LoadDll("r8bsrc.dll"))
		, r8b_create(module_, "r8b_create")
		, r8b_clear(module_, "r8b_clear")
		, r8b_delete(module_, "r8b_delete")
		, r8b_process(module_, "r8b_process") {
	}
	catch (const Exception & e) {
		XAMP_LOG_ERROR("{}", e.GetErrorMessage());
	}

	static XAMP_ALWAYS_INLINE CdspLib& Instance() {
		static CdspLib lib;
		return lib;
	}

	XAMP_DISABLE_COPY(CdspLib)

private:
	ModuleHandle module_;

public:	
	XAMP_DEFINE_DLL_API(r8b_create) r8b_create;
	XAMP_DEFINE_DLL_API(r8b_clear) r8b_clear;
	XAMP_DEFINE_DLL_API(r8b_delete) r8b_delete;
	XAMP_DEFINE_DLL_API(r8b_process) r8b_process;
};

constexpr const double ReqTransBand = 2.0;
constexpr const double ReqAtten = 206.91;

class CdspResampler::CdspResamplerImpl {
public:
	CdspResamplerImpl()
		: input_samplerate_(0)
		, output_samplerate_(0) {
		resampler_.fill(nullptr);
	}

	~CdspResamplerImpl() {
		Close();
	}

	void Start(int32_t input_samplerate, int32_t num_channels, int32_t output_samplerate, int32_t max_sample) {
		Clear();

		buffer_.resize(num_channels);
		for (size_t i = 0; i < 2; ++i) {
			resampler_[i] = CdspLib::Instance().r8b_create(input_samplerate,
				output_samplerate, 
				max_sample / 2, 
				ReqTransBand,
				ER8BResamplerRes::r8brr24);
			buffer_[i].resize(max_sample);
		}
		input_samplerate_ = input_samplerate;
		output_samplerate_ = output_samplerate;
	}

	bool Process(const float* samples, int32_t num_sample, AudioBuffer<int8_t>& buffer) {
		double* output_samples[2];
		
		for (int32_t i = 0; i < num_sample / 2; ++i) {
			buffer_[0][i] = static_cast<double>(samples[i * 2]);
			buffer_[1][i] = static_cast<double>(samples[i * 2 + 1]);
		}
	
		int32_t write_count = 0;
		for (size_t i = 0; i < 2; ++i) {
			write_count = CdspLib::Instance().r8b_process(resampler_[i], buffer_[i].data(), num_sample / 2, output_samples[i]);
		}

		write_buffer_.resize(write_count * 2);

		for (int32_t i = 0; i < write_count; ++i) {
			write_buffer_[i * 2] = static_cast<float>(output_samples[0][i]);
			write_buffer_[i * 2 + 1] = static_cast<float>(output_samples[1][i]);
		}

		buffer.TryWrite(reinterpret_cast<int8_t*>(write_buffer_.data()), write_buffer_.size() * sizeof(float));
		return true;
	}

	void Clear() {
		for (auto& resampler : resampler_) {
			if (resampler != nullptr) {
				CdspLib::Instance().r8b_clear(resampler);
			}
		}
	}

	void Close() {
		for (auto& resampler : resampler_) {
			if (resampler != nullptr) {
				CdspLib::Instance().r8b_delete(resampler);
				resampler = nullptr;
			}
		}
	}

	int32_t input_samplerate_;
	int32_t output_samplerate_;
	std::array<CR8BResampler, 2> resampler_;
	std::vector<std::vector<double>> buffer_;
	std::vector<float> write_buffer_;
};

CdspResampler::CdspResampler()
	: impl_(MakeAlign<CdspResamplerImpl>()) {
}

XAMP_PIMPL_IMPL(CdspResampler)

void CdspResampler::LoadCdspLib() {
	CdspLib::Instance();
}

std::string_view CdspResampler::GetDescription() const noexcept {
	return "r8bsrc";
}

void CdspResampler::Start(int32_t input_samplerate, int32_t num_channels, int32_t output_samplerate, int32_t max_sample) {
	impl_->Start(input_samplerate, num_channels, output_samplerate, max_sample);
}

bool CdspResampler::Process(const float* samples, int32_t num_sample, AudioBuffer<int8_t>& buffer) {
	return impl_->Process(samples, num_sample, buffer);
}

}
