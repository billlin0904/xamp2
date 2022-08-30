#include <fstream>
#include <vector>

#include <base/exception.h>
#include <base/memory.h>
#include <base/logger_impl.h>
#include <base/int24.h>
#include <base/audioformat.h>
#include <base/platform.h>

#include <stream/fftwlib.h>
#include <stream/dsd_utils.h>
#include <stream/pcm2dsdsamplewriter.h>

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

class Pcm2DsdSampleWriter::Pcm2DsdSampleWriterImpl {
public:
	explicit Pcm2DsdSampleWriterImpl(DsdTimes dsd_times) {
		dsd_times_ = static_cast<uint32_t>(dsd_times);
		logger_ = LoggerManager::GetInstance().GetLogger("Pcm2DsdConverter");
	}

	void Init(uint32_t input_sample_rate) {
		uint32_t times = 0;
	
		auto dsd_times = pow(2, dsd_times_);

		if (input_sample_rate % 44100 == 0) {
			times = dsd_times / (input_sample_rate / 44100);
			dsd_sampling_rate_ = input_sample_rate * times;
		}
		else {
			times = dsd_times / (input_sample_rate / 48000);
			dsd_sampling_rate_ = input_sample_rate * times;
		}

		XAMP_LOG_D(logger_, "DSD Times: {} Times: {} DsdSampleRate: {}", dsd_times, times, dsd_sampling_rate_);

		InitFFT(times);
	}

	uint32_t GetDsdSampleRate() const {
		return dsd_sampling_rate_;
	}

	uint32_t GetDsdSpeed() const {
		return dsd_sampling_rate_ / kPcmSampleRate441;
	}

	void SplitChannel(float const* samples, size_t num_samples) {
		if (lch_src_.size() != num_samples / AudioFormat::kMaxChannel) {
			lch_src_.resize(num_samples / AudioFormat::kMaxChannel);
		}

		if (rch_src_.size() != num_samples / AudioFormat::kMaxChannel) {
			rch_src_.resize(num_samples / AudioFormat::kMaxChannel);
		}

		for (auto i = 0; i < num_samples / AudioFormat::kMaxChannel; ++i) {
			lch_src_[i] = samples[i * 2 + 0];
			rch_src_[i] = samples[i * 2 + 1];
		}
	}

	void ProcessChannel(const std::vector<double> &channel, std::fstream &output_file) {
		const auto datasize = fft_size_ / 2;
		auto split_num = (channel.size() / datasize) * dsd_times_;
		auto data = reinterpret_cast<const uint8_t*>(channel.data());
		auto updata_size = datasize * dsd_times_;

		double gain = 1;
		double deltagain = 0.5;
		deltagain = gain * deltagain;
		auto order = NoiseShapingCoeff()[0].size();
		auto p = 0;
		auto t = 0;			
		auto q = 0;

		XAMP_LOG_D(logger_, "Split count: {}, data size: {}", split_num, datasize);

		for (auto i = 0; i < split_num; ++i) {
			MemoryCopy(buffer_.data(), data, 8 * (datasize / dsd_times_));

			for (t = 0; t < log_times_; t++) {
				q = 0;
				for (p = 0; p < zero_size_[t]; p++) {
					fftin_[t][q] = buffer_[p];
					q++;
					fftin_[t][q] = 0;
					q++;
				}

				MemorySet(fftin_[t].get() + q, 0, 8 * (nowfft_size_[t] - q));

				FFTW_LIB.fftw_execute(fft_[t].get());
				for (p = 0; p < realfft_size_[t]; p++) {
					ifftin_[t][p][0] = fftout_[t][p][0] * firfilter_fft_[t][p][0] - fftout_[t][p][1] * firfilter_fft_[t][p][1];
					ifftin_[t][p][1] = fftout_[t][p][0] * firfilter_fft_[t][p][1] + firfilter_fft_[t][p][0] * fftout_[t][p][1];
				}

				FFTW_LIB.fftw_execute(ifft_[t].get());
				for (p = 0; p < pudding_size_[t]; p++) {
					buffer_[p] = prebuffer_[t][p] + ifftout_[t][p];
				}

				q = 0;
				for (p = pudding_size_[t]; p < nowfft_size_[t]; p++) {
					buffer_[p] = prebuffer_[t][q] = ifftout_[t][p];
					q++;
				}
			}

			double error_y = 0;
			double x_in = 0;

			for (q = 0; q < datasize; q++) {
				x_in = buffer_[q] * deltagain;

				for (t = 0; t < order; t++) {
					x_in += NoiseShapingCoeff()[0][t] * delta_buffer_[t];
				}

				if (x_in >= 0.0) {
					temp_out_[q] = 1;
					error_y = -1.0;
				}
				else {
					temp_out_[q] = 0;
					error_y = 1.0;
				}
				for (t = order; t > 0; t--) {
					delta_buffer_[t] = delta_buffer_[t - 1];
				}

				delta_buffer_[0] = x_in + error_y;

				for (t = 0; t < order; t++) {
					delta_buffer_[0] += NoiseShapingCoeff()[1][t] * delta_buffer_[t + 1];
				}
			}

			output_file.write(reinterpret_cast<const char*>(temp_out_.data()), datasize);
			data += 8 * (datasize / dsd_times_);
		}
	}

	void CreateTempFile() {
		auto temp_path = Fs::temp_directory_path();
		lch_out_path_ = temp_path / MakeTempFileName();
		lch_out_.open(lch_out_path_, std::ios::binary);
		rch_out_path_ = temp_path / MakeTempFileName();
		rch_out_.open(rch_out_path_, std::ios::binary);
	}

	void RemoveTempFile() {
		Fs::remove(lch_out_path_);
		Fs::remove(rch_out_path_);
	}

	bool MargeChannel(AudioBuffer<int8_t>& buffer) {
		constexpr auto buffersize = 16384 * 2 * 8;

		auto p = 0;
		auto i = 0;

		std::vector<uint8_t> onebit(buffersize / 4);
		std::vector<uint8_t> tmpdataL(buffersize);
		std::vector<uint8_t> tmpdataR(buffersize);
		
		for (; i < dsd_sampling_rate_ / buffersize; i++) {
			p = 0;
			lch_out_.read(reinterpret_cast<char*>(tmpdataL.data()), tmpdataL.size());
			rch_out_.read(reinterpret_cast<char*>(tmpdataR.data()), tmpdataR.size());
			for (auto k = 0; k < buffersize / 4; k++) {
				onebit[k] = tmpdataL[p] << 7;
				onebit[k] += tmpdataL[p + 1] << 6;
				onebit[k] += tmpdataL[p + 2] << 5;
				onebit[k] += tmpdataL[p + 3] << 4;
				onebit[k] += tmpdataL[p + 4] << 3;
				onebit[k] += tmpdataL[p + 5] << 2;
				onebit[k] += tmpdataL[p + 6] << 1;
				onebit[k] += tmpdataL[p + 7] << 0;
				k++;
				onebit[k] = tmpdataR[p] << 7;
				onebit[k] += tmpdataR[p + 1] << 6;
				onebit[k] += tmpdataR[p + 2] << 5;
				onebit[k] += tmpdataR[p + 3] << 4;
				onebit[k] += tmpdataR[p + 4] << 3;
				onebit[k] += tmpdataR[p + 5] << 2;
				onebit[k] += tmpdataR[p + 6] << 1;
				onebit[k] += tmpdataR[p + 7] << 0;
				p += 8;
			}
			buffer.TryWrite(reinterpret_cast<int8_t*>(onebit.data()), buffersize / 4);
		}

		RemoveTempFile();
		return true;
	}

	bool Process(float const* samples, size_t num_samples, AudioBuffer<int8_t>& buffer) {
		CreateTempFile();
		SplitChannel(samples, num_samples);		
		ProcessChannel(lch_src_, lch_out_);
		ProcessChannel(rch_src_, rch_out_);
		return MargeChannel(buffer);
	}

	void InitFFT(uint32_t times) {
		FIRFilter();
		NoiseShapingCoeff();

		const auto section_1 = FIRFilter().size();
		const auto logtimes = static_cast<uint32_t>(log(times) / log(2));
		const auto fftsize = (section_1 + 1) * times;
		const auto datasize = fftsize / 2;
		
		log_times_ = logtimes;
		fft_size_ = fftsize;
		dsd_times_ = times;

		nowfft_size_.resize(logtimes);
		zero_size_.resize(logtimes);
		pudding_size_.resize(logtimes);
		realfft_size_.resize(logtimes);
		add_size_.resize(logtimes);
		prebuffer_.resize(logtimes);

		fftin_.resize(logtimes);
		fftout_.resize(logtimes);
		ifftout_.resize(logtimes);
		ifftin_.resize(logtimes);
		firfilter_fft_.resize(logtimes);

		fft_.resize(logtimes);
		ifft_.resize(logtimes);

		temp_out_.resize(datasize);

		buffer_.resize(fftsize);
		delta_buffer_.resize(NoiseShapingCoeff()[0].size() + 1);

		double gain = 1;
		auto i = 0;
		auto p = 0;
		auto k = 0;
		for (i = 1; i < times; i = i * 2) {
			nowfft_size_[p] = fftsize / (times / (i * 2));
			realfft_size_[p] = nowfft_size_[p] / 2 + 1;
			zero_size_[p] = nowfft_size_[p] / 4;
			pudding_size_[p] = nowfft_size_[p] - zero_size_[p] * 2;
			gain = gain * (2.0 / nowfft_size_[p]);
			add_size_[p] = zero_size_[p] * 2;

			prebuffer_[p] = MakeFFTWDoubleArray(fftsize);
			firfilter_fft_[logtimes - p - 1] = MakeFFTWComplexArray(fftsize / i);
			fftin_[logtimes - p - 1] = MakeFFTWDoubleArray(fftsize / i);
			fftout_[logtimes - p - 1] = MakeFFTWComplexArray(fftsize / i);
			ifftin_[logtimes - p - 1] = MakeFFTWComplexArray((fftsize / i + 1) / 2 + 1);
			ifftout_[logtimes - p - 1] = MakeFFTWDoubleArray(fftsize / i);

			for (k = 0; k < fftsize / i; k++) {
				fftin_[logtimes - p - 1][k] = 0;
				ifftout_[logtimes - p - 1][k] = 0;
				fftout_[logtimes - p - 1][k][0] = 0;
				fftout_[logtimes - p - 1][k][1] = 0;
				ifftin_[logtimes - p - 1][k / 2][0] = 0;
				ifftin_[logtimes - p - 1][k / 2][1] = 0;
			}
			for (k = 0; k < fftsize; k++) {
				prebuffer_[p][k] = 0;
			}
			++p;
		}

		p = 0;
		for (i = 1; i < times; i = i * 2) {
			fft_[logtimes - p - 1] = MakeFFTW(fftsize, i, fftin_[logtimes - p - 1].get(), fftout_[logtimes - p - 1].get());
			ifft_[logtimes - p - 1] = MakeIFFTW(fftsize, i, ifftin_[logtimes - p - 1].get(), ifftout_[logtimes - p - 1].get());
			++p;
		}

		for (k = 0; k < logtimes; k++) {
			for (auto i = 0; i < section_1; i++) {
				fftin_[k][i] = FIRFilter()[i];
			}
			for (auto i = section_1; i < fftsize / pow(2, k); i++) {
				fftin_[logtimes - k - 1][i] = 0;
			}
		}

		for (i = 0; i < logtimes; i++) {
			FFTW_LIB.fftw_execute(fft_[logtimes - i - 1].get());
			for (p = 0; p < fftsize / pow(2, i + 1) + 1; p++) {
				firfilter_fft_[logtimes - i - 1][p][0] = fftout_[logtimes - i - 1][p][0];
				firfilter_fft_[logtimes - i - 1][p][1] = fftout_[logtimes - i - 1][p][1];
			}
		}

		for (i = 0; i < logtimes; i++) {
			FFTW_LIB.fftw_execute(fft_[logtimes - i - 1].get());
			for (p = 0; p < fftsize / pow(2, i + 1) + 1; p++) {
				firfilter_fft_[logtimes - i - 1][p][0] = fftout_[logtimes - i - 1][p][0];
				firfilter_fft_[logtimes - i - 1][p][1] = fftout_[logtimes - i - 1][p][1];
			}
		}
	}

	uint32_t dsd_sampling_rate_{ 0 };
	uint32_t dsd_times_{ 0 };
	uint32_t log_times_{ 0 };
	uint32_t fft_size_{ 0 };
	std::vector<int8_t> temp_out_;
	Path lch_out_path_;
	Path rch_out_path_;
	std::fstream lch_out_;
	std::fstream rch_out_;
	std::vector<uint32_t> nowfft_size_;
	std::vector<uint32_t> zero_size_;
	std::vector<uint32_t> pudding_size_;
	std::vector<uint32_t> realfft_size_;
	std::vector<uint32_t> add_size_;
	std::vector<FFTWPlan> fft_;
	std::vector<FFTWPlan> ifft_;
	std::vector<FFTWDoublePtr> fftin_;
	std::vector<FFTWComplexPtr> fftout_;
	std::vector<FFTWDoublePtr> ifftout_;
	std::vector<FFTWComplexPtr> ifftin_;
	std::vector<FFTWDoublePtr> prebuffer_;
	std::vector<FFTWComplexPtr> firfilter_fft_;
	std::vector<double> buffer_;
	std::vector<double> delta_buffer_;
	std::vector<double> lch_src_;
	std::vector<double> rch_src_;
	std::shared_ptr<Logger> logger_;
};

XAMP_PIMPL_IMPL(Pcm2DsdSampleWriter)

Pcm2DsdSampleWriter::Pcm2DsdSampleWriter(DsdTimes dsd_times)
	: impl_(MakeAlign<Pcm2DsdSampleWriterImpl>(dsd_times)) {
}

void Pcm2DsdSampleWriter::Init(uint32_t output_sample_rate) {
	impl_->Init(output_sample_rate);
}

uint32_t Pcm2DsdSampleWriter::GetDsdSampleRate() const {
	return impl_->GetDsdSampleRate();
}

uint32_t Pcm2DsdSampleWriter::GetDsdSpeed() const {
	return impl_->GetDsdSpeed();
}

[[nodiscard]] bool Pcm2DsdSampleWriter::Process(BufferRef<float> const& input, AudioBuffer<int8_t>& buffer) {
	return Process(input.data(), input.size(), buffer);
}

bool Pcm2DsdSampleWriter::Process(float const* samples, size_t num_samples, AudioBuffer<int8_t>& buffer) {
	return impl_->Process(samples, num_samples, buffer);
}

#else

class Pcm2DsdConverter::Pcm2DsdSampleWriterImpl {
public:
	Pcm2DsdSampleWriterImpl(DsdTimes dsd_times) {
    }
};

XAMP_PIMPL_IMPL(Pcm2DsdSampleWriter)

Pcm2DsdSampleWriter::Pcm2DsdSampleWriter(DsdTimes dsd_times) {
}

void Pcm2DsdSampleWriter::Init(uint32_t output_sample_rate, uint32_t dsd_times) {
}

[[nodiscard]] bool Pcm2DsdSampleWriter::Process(BufferRef<float> const& input, AudioBuffer<int8_t>& buffer) {
    return Process(input.data(), input.size(), buffer);
}

bool Pcm2DsdSampleWriter::Process(float const* samples, size_t num_samples, AudioBuffer<int8_t>& buffer) {
    return false;
}

#endif

}
