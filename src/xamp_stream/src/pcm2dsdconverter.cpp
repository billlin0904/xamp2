#include <fstream>
#include <base/exception.h>
#include <base/memory.h>
#include <stream/fftwlib.h>
#include <stream/pcm2dsdconverter.h>

namespace xamp::stream {

#ifdef XAMP_OS_WIN

static const std::vector<double> & FIRFilter() {
	static constexpr auto readFIRFilter = []() {
		static std::vector<double> data;

		std::ifstream file(".\\FIRFilter.dat");
		if (file.fail()) {
			throw Exception();
		}

		std::string str;
		std::getline(file, str);
		auto filter_size = atoi(str.c_str());
		data.reserve(filter_size);

		while (std::getline(file, str)) {
			data.push_back(atof(str.c_str()));
		}
		return data;
	};

	static const auto lut = readFIRFilter();
	return lut;
}

static const std::vector<std::vector<double>>& NoiseShapingCoeff() {	
	static constexpr auto readNoiseShapingCoeff = []() {
		static std::vector<std::vector<double>> data;

		std::ifstream file(".\\NoiseShapingCoeff.dat");
		if (file.fail()) {
			throw Exception();
		}

		std::string str;
		auto i = 0; int s = 0;
		std::getline(file, str);

		auto order = atoi(str.c_str());
		data.resize(2);
		data[0].resize(order);
		data[1].resize(order);

		while (std::getline(file, str)) {
			if (str != "0") {
				if (i == 0)
					data[i][s] = atof(str.c_str());
				else {
					data[i][order - s - 1] = atof(str.c_str());
				}
				s++;
			}
			else {
				s = 0;
				i++;
			}
		}

		for (i = 0; i < order; i++) {
			data[1][i] = data[1][i];
			data[0][i] = data[0][i] - data[1][i];
		}
		return data;
	};

	static const auto lut = readNoiseShapingCoeff();
	return lut;
}

static FFTWComplexPtr MakeFFTWComplexArray(size_t size) {
	return FFTWComplexPtr(static_cast<fftw_complex*>(FFTW_LIB.fftw_malloc(sizeof(fftw_complex) * size)));
}

static FFTWDoublePtr MakeFFTWDoubleArray(size_t size) {
	return FFTWDoublePtr(static_cast<double*>(FFTW_LIB.fftw_malloc(sizeof(double) * size)));
}

static FFTWPlan MakeFFTW(uint32_t fftsize, uint32_t times, double* fftin, fftw_complex* fftout) {
	return FFTWPlan(FFTW_LIB.fftw_plan_dft_r2c_1d(fftsize / times, fftin, fftout, FFTW_ESTIMATE));
}

static FFTWPlan MakeIFFTW(uint32_t fftsize, uint32_t times, fftw_complex* ifftin, double* ifftout) {
	return FFTWPlan(FFTW_LIB.fftw_plan_dft_c2r_1d(fftsize / times, ifftin, ifftout, FFTW_ESTIMATE));
}

class Pcm2DsdConverter::Pcm2DsdConverterImpl {
public:
	Pcm2DsdConverterImpl() {		
	}

	void Init(uint32_t output_sample_rate, uint32_t dsd_times) {
		uint32_t times = 0;
		uint32_t dsd_sampling_rate = 0;

		if (output_sample_rate % 44100 == 0) {
			times = dsd_times / (output_sample_rate / 44100);
			dsd_sampling_rate = output_sample_rate * times;
		}
		else {
			times = dsd_times / (output_sample_rate / 48000);
			dsd_sampling_rate = output_sample_rate * times;
		}

		Init(times);
	}

	void SplitChannel(float const* samples, size_t num_samples) {
		if (lch_src_.size() != num_samples / 2) {
			lch_src_.resize(num_samples / 2);
		}
		if (rch_src_.size() != num_samples / 2) {
			rch_src_.resize(num_samples / 2);
		}
		for (auto i = 0; i < num_samples / 2; ++i) {
			lch_src_[i] = samples[i];
			rch_src_[i] = samples[i + 1];
		}
	}

	void ProcessChannel(const std::vector<double> &channel, std::vector<int8_t>& out) {
		const auto datasize = fftsize_ / 2;
		auto split_num = (channel.size() / datasize) * dsd_times_;
		auto data = reinterpret_cast<const uint8_t*>(channel.data());

		double deltagain = 0.5;
		auto order = NoiseShapingCoeff().size();

		for (auto i = 0; i < split_num; ++i) {
			MemoryCopy(buffer_.data(), data, 8 * (datasize / dsd_times_));
			for (auto t = 0; t < logtimes_; t++) {
				auto q = 0;
				for (auto p = 0; p < zerosize_[t]; p++) {
					fftin_[t][q] = buffer_[p];
					q++;
					fftin_[t][q] = 0;
					q++;
				}
				MemorySet(fftin_[t].get() + q, 0, 8 * (nowfftsize_[t] - q));
				FFTW_LIB.fftw_execute(fft_[t].get());
				for (auto p = 0; p < realfftsize_[t]; p++) {
					ifftin_[t][p][0] = fftout_[t][p][0] * firfilter_fft_[t][p][0] - fftout_[t][p][1] * firfilter_fft_[t][p][1];
					ifftin_[t][p][1] = fftout_[t][p][0] * firfilter_fft_[t][p][1] + firfilter_fft_[t][p][0] * fftout_[t][p][1];
				}
				FFTW_LIB.fftw_execute(ifft_[t].get());
				for (auto p = 0; p < puddingsize_[t]; p++) {
					buffer_[p] = prebuffer_[t][p] + ifftout_[t][p];
				}
				q = 0;
				for (auto p = puddingsize_[t]; p < nowfftsize_[t]; p++) {
					buffer_[p] = prebuffer_[t][q] = ifftout_[t][p];
					q++;
				}
			}

			for (auto q = 0; q < datasize; q++) {
				double x_in = buffer_[q] * deltagain;

				for (auto t = 0; t < order; t++) {
					x_in += NoiseShapingCoeff()[0][t] * deltabuffer_[t];
				}

				double error_y = 0;

				if (x_in >= 0.0) {
					out[q] = 1;
					error_y = -1.0;
				}
				else {
					out[q] = 0;
					error_y = 1.0;
				}
				for (auto t = order; t > 0; t--) {
					deltabuffer_[t] = deltabuffer_[t - 1];
				}

				deltabuffer_[0] = x_in + error_y;

				for (auto t = 0; t < order; t++) {
					deltabuffer_[0] += NoiseShapingCoeff()[1][t] * deltabuffer_[t + 1];
				}
			}

			data += 8 * (datasize / dsd_times_);
		}
	}

	bool Process(float const* samples, size_t num_samples, AudioBuffer<int8_t>& buffer) {
		SplitChannel(samples, num_samples);
		ProcessChannel(lch_src_, lch_out_);
		ProcessChannel(rch_src_, rch_out_);
		for (auto i = 0; i < out_.size() / 2; ++i) {
			out_[i + 0] = lch_out_[i];
			out_[i + 1] = rch_out_[i];
		}
		BufferOverFlowThrow(buffer.TryWrite(out_.data(), out_.size()));
		return true;
	}

	void Init(uint32_t times) {
		FIRFilter();
		NoiseShapingCoeff();

		const auto section_1 = FIRFilter().size();
		const auto logtimes = static_cast<uint32_t>(log(times) / log(2));
		const auto fftsize = (section_1 + 1) * times;
		const auto datasize = fftsize / 2;

		logtimes_ = logtimes;
		fftsize_ = fftsize;
		dsd_times_ = times;

		nowfftsize_.resize(logtimes);
		zerosize_.resize(logtimes);
		puddingsize_.resize(logtimes);
		realfftsize_.resize(logtimes);
		addsize_.resize(logtimes);
		prebuffer_.resize(logtimes);

		fftin_.resize(logtimes);
		fftout_.resize(logtimes);
		ifftout_.resize(logtimes);
		ifftin_.resize(logtimes);
		firfilter_fft_.resize(logtimes);

		fft_.resize(logtimes);
		ifft_.resize(logtimes);

		buffer_.resize(fftsize);
		deltabuffer_.resize(NoiseShapingCoeff().size() + 1);
		lch_out_.resize(datasize);
		rch_out_.resize(datasize);
		out_.resize(datasize * 2);

		double gain = 1;
		uint32_t p = 0;

		for (auto i = 1; i < times; i = i * 2) {
			nowfftsize_[p] = fftsize / (times / (i * 2));
			realfftsize_[p] = nowfftsize_[p] / 2 + 1;
			zerosize_[p] = nowfftsize_[p] / 4;
			puddingsize_[p] = nowfftsize_[p] - zerosize_[p] * 2;
			gain = gain * (2.0 / nowfftsize_[p]);
			addsize_[p] = zerosize_[p] * 2;

			prebuffer_[p] = MakeFFTWDoubleArray(fftsize);
			firfilter_fft_[logtimes - p - 1] = MakeFFTWComplexArray(fftsize / i);
			fftin_[logtimes - p - 1] = MakeFFTWDoubleArray(fftsize / i);
			fftout_[logtimes - p - 1] = MakeFFTWComplexArray(fftsize / i);
			ifftin_[logtimes - p - 1] = MakeFFTWComplexArray((fftsize / i + 1) / 2 + 1);
			ifftout_[logtimes - p - 1] = MakeFFTWDoubleArray(fftsize / i);

			for (auto k = 0; k < fftsize / i; k++) {
				fftin_[logtimes - p - 1][k] = 0;
				ifftout_[logtimes - p - 1][k] = 0;
				fftout_[logtimes - p - 1][k][0] = 0;
				fftout_[logtimes - p - 1][k][1] = 0;
				ifftin_[logtimes - p - 1][k / 2][0] = 0;
				ifftin_[logtimes - p - 1][k / 2][1] = 0;
			}
			for (auto k = 0; k < fftsize; k++) {
				prebuffer_[p][k] = 0;
			}
			++p;
		}

		p = 0;
		for (auto i = 1; i < times; i = i * 2) {
			fft_[logtimes - p - 1] = MakeFFTW(fftsize, i, fftin_[logtimes - p - 1].get(), fftout_[logtimes - p - 1].get());
			ifft_[logtimes - p - 1] = MakeIFFTW(fftsize, i, ifftin_[logtimes - p - 1].get(), ifftout_[logtimes - p - 1].get());
			++p;
		}

		for (auto k = 0; k < logtimes; k++) {
			for (auto i = 0; i < section_1; i++) {
				fftin_[k][i] = FIRFilter()[i];
			}
			for (auto i = section_1; i < fftsize / pow(2, k); i++) {
				fftin_[logtimes - k - 1][i] = 0;
			}
		}

		for (auto i = 0; i < logtimes; i++) {
			FFTW_LIB.fftw_execute(fft_[logtimes - i - 1].get());
			for (p = 0; p < fftsize / pow(2, i + 1) + 1; p++) {
				firfilter_fft_[logtimes - i - 1][p][0] = fftout_[logtimes - i - 1][p][0];
				firfilter_fft_[logtimes - i - 1][p][1] = fftout_[logtimes - i - 1][p][1];
			}
		}

		for (auto i = 0; i < logtimes; i++) {
			FFTW_LIB.fftw_execute(fft_[logtimes - i - 1].get());
			for (p = 0; p < fftsize / pow(2, i + 1) + 1; p++) {
				firfilter_fft_[logtimes - i - 1][p][0] = fftout_[logtimes - i - 1][p][0];
				firfilter_fft_[logtimes - i - 1][p][1] = fftout_[logtimes - i - 1][p][1];
			}
		}
	}

	uint32_t logtimes_{ 0 };
	uint32_t fftsize_{ 0 };
	uint32_t dsd_times_{ 0 };
	std::vector<int8_t> lch_out_;
	std::vector<int8_t> rch_out_;
	std::vector<int8_t> out_;
	std::vector<double> buffer_;
	std::vector<double> deltabuffer_;
	std::vector<FFTWPlan> fft_;
	std::vector<FFTWPlan> ifft_;
	std::vector<FFTWDoublePtr> fftin_;
	std::vector<FFTWComplexPtr> fftout_;
	std::vector<FFTWDoublePtr> ifftout_;
	std::vector<FFTWComplexPtr> ifftin_;
	std::vector<FFTWDoublePtr> prebuffer_;
	std::vector<FFTWComplexPtr> firfilter_fft_;
	std::vector<uint32_t> nowfftsize_;
	std::vector<uint32_t> zerosize_;
	std::vector<uint32_t> puddingsize_;
	std::vector<uint32_t> realfftsize_;
	std::vector<uint32_t> addsize_;
	std::vector<double> lch_src_;
	std::vector<double> rch_src_;
};

XAMP_PIMPL_IMPL(Pcm2DsdConverter)

Pcm2DsdConverter::Pcm2DsdConverter()
	: impl_(MakeAlign<Pcm2DsdConverterImpl>()) {
}

void Pcm2DsdConverter::Init(uint32_t output_sample_rate, uint32_t dsd_times) {
	impl_->Init(output_sample_rate, dsd_times);
}

[[nodiscard]] std::string_view Pcm2DsdConverter::GetDescription() const noexcept {
	return "";
}

[[nodiscard]] bool Pcm2DsdConverter::Process(BufferRef<float> const& input, AudioBuffer<int8_t>& buffer) {
	return Process(input.data(), input.size(), buffer);
}

bool Pcm2DsdConverter::Process(float const* samples, size_t num_samples, AudioBuffer<int8_t>& buffer) {
	return impl_->Process(samples, num_samples, buffer);
}

#else

class Pcm2DsdConverter::Pcm2DsdConverterImpl {
public:
    Pcm2DsdConverterImpl() {
    }
};

XAMP_PIMPL_IMPL(Pcm2DsdConverter)

Pcm2DsdConverter::Pcm2DsdConverter() {
}

void Pcm2DsdConverter::Init(uint32_t output_sample_rate, uint32_t dsd_times) {
}

[[nodiscard]] std::string_view Pcm2DsdConverter::GetDescription() const noexcept {
    return "";
}

[[nodiscard]] bool Pcm2DsdConverter::Process(BufferRef<float> const& input, AudioBuffer<int8_t>& buffer) {
    return Process(input.data(), input.size(), buffer);
}

bool Pcm2DsdConverter::Process(float const* samples, size_t num_samples, AudioBuffer<int8_t>& buffer) {
    return false;
}

#endif

}
