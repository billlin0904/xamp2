#include <fstream>
#include <vector>

#include <base/exception.h>
#include <base/memory.h>
#include <base/logger_impl.h>
#include <base/int24.h>
#include <base/audioformat.h>
#include <base/platform.h>
#include <base/assert.h>
#include <base/fastmutex.h>
#include <base/ithreadpool.h>

#include <stream/fftwlib.h>
#include <stream/dsd_utils.h>
#include <stream/pcm2dsdsamplewriter.h>

namespace xamp::stream {

#ifdef XAMP_OS_WIN

class Double2DArray {
public:
	Double2DArray() noexcept
		: length_(0)
		, width_(0) {
	}

	Double2DArray(size_t length, size_t width) {
		Resize(length, width);
	}

	size_t GetLength() const noexcept {
		return length_;
	}

	void Resize(size_t length, size_t width) {
		length_ = length;
		width_ = width;
		data_.resize(length * width);
	}

	void Set(uint32_t x, uint32_t y, double val) {
		Get(x, y) = val;
	}

	double & Get(uint32_t x, uint32_t y) {
		return data_[x + y * width_];
	}

	const double& Get(uint32_t x, uint32_t y) const {
		return data_[x + y * width_];
	}
private:
	size_t length_;
	size_t width_;
	std::vector<double> data_;
};

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

static const Double2DArray& NoiseShapingCoeff() {
	static constexpr auto readNoiseShapingCoeff = []() {
		static Double2DArray data;

		std::ifstream file(".\\NoiseShapingCoeff.dat");
		if (file.fail()) {
			throw Exception();
		}

		std::string str;
		auto i = 0; int s = 0;
		std::getline(file, str);

		auto order = atoi(str.c_str());
		data.Resize(order, 2);

		while (std::getline(file, str)) {
			if (str != "0") {
				if (i == 0)
					data.Set(i, s, atof(str.c_str()));
				else {
					data.Set(i, order - s - 1, atof(str.c_str()));
				}
				s++;
			}
			else {
				s = 0;
				i++;
			}
		}

		for (i = 0; i < order; i++) {
			data.Set(1, i, data.Get(1, i));
			data.Set(0, i, data.Get(0, i) - data.Get(1, i));
		}
		return data;
	};

	static const auto lut = readNoiseShapingCoeff();
	return lut;
}

struct FFTWContext {
	void Init(uint32_t log_times, 
		uint32_t times,
		uint32_t fft_size,
		uint32_t section_1,
		std::shared_ptr<Logger> logger) {
		nowfft_size.resize(log_times);
		zero_size.resize(log_times);
		pudding_size.resize(log_times);
		realfft_size.resize(log_times);
		add_size.resize(log_times);
		prebuffer.resize(log_times);
		fftin.resize(log_times);
		fftout.resize(log_times);
		ifftout.resize(log_times);
		ifftin.resize(log_times);
		firfilter_fft.resize(log_times);
		fft.resize(log_times);
		ifft.resize(log_times);

		XAMP_LOG_D(logger, "Process FFT/IFFT");

		const auto& firfilter_table = FIRFilter();
		auto i = 0u;
		auto p = 0u;
		auto k = 0u;

		for (i = 1u; i < times; i = i * 2) {
			nowfft_size[p] = fft_size / (times / (i * 2));
			realfft_size[p] = nowfft_size[p] / 2 + 1;
			zero_size[p] = nowfft_size[p] / 4;
			pudding_size[p] = nowfft_size[p] - zero_size[p] * 2;
			gain = gain * (2.0 / nowfft_size[p]);
			add_size[p] = zero_size[p] * 2;

			XAMP_LOG_D(logger,
				"Make FFT/IFFT I/O {} (log_times: {} p : {} i : {}) Buffer",
				log_times - p - 1, log_times, p, i);

			prebuffer[p] = MakeFFTWDoubleArray(fft_size);
			firfilter_fft[log_times - p - 1] = MakeFFTWComplexArray(fft_size / i);
			fftin[log_times - p - 1] = MakeFFTWDoubleArray(fft_size / i);
			fftout[log_times - p - 1] = MakeFFTWComplexArray(fft_size / i);
			ifftin[log_times - p - 1] = MakeFFTWComplexArray((fft_size / i + 1) / 2 + 1);
			ifftout[log_times - p - 1] = MakeFFTWDoubleArray(fft_size / i);

			for (k = 0u; k < fft_size / i; k++) {
				fftin[log_times - p - 1][k] = 0;
				ifftout[log_times - p - 1][k] = 0;
				fftout[log_times - p - 1][k][0] = 0;
				fftout[log_times - p - 1][k][1] = 0;
				ifftin[log_times - p - 1][k / 2][0] = 0;
				ifftin[log_times - p - 1][k / 2][1] = 0;
			}
			for (k = 0u; k < fft_size; k++) {
				prebuffer[p][k] = 0;
			}
			++p;
		}

		XAMP_LOG_D(logger, "Make FFT/IFFT Buffer");

		p = 0;
		for (i = 1u; i < times; i = i * 2) {
			fft[log_times - p - 1] = MakeFFTW(fft_size, i, fftin[log_times - p - 1], fftout[log_times - p - 1]);
			ifft[log_times - p - 1] = MakeIFFTW(fft_size, i, ifftin[log_times - p - 1], ifftout[log_times - p - 1]);
			XAMP_ASSERT(fft[log_times - p - 1]);
			XAMP_ASSERT(ifft[log_times - p - 1]);
			++p;
		}

		for (k = 0u; k < log_times; k++) {
			for (i = 0u; i < section_1; i++) {
				fftin[k][i] = firfilter_table[i];
			}
			for (i = section_1; i < fft_size / pow(2, k); i++) {
				fftin[log_times - k - 1][i] = 0;
			}
		}

		for (i = 0u; i < log_times; i++) {
			FFTW_LIB.fftw_execute(fft[log_times - i - 1].get());
			for (p = 0; p < fft_size / pow(2, i + 1) + 1; p++) {
				firfilter_fft[log_times - i - 1][p][0] = fftout[log_times - i - 1][p][0];
				firfilter_fft[log_times - i - 1][p][1] = fftout[log_times - i - 1][p][1];
			}
		}

		for (i = 0u; i < log_times; i++) {
			FFTW_LIB.fftw_execute(fft[log_times - i - 1].get());
			for (p = 0; p < fft_size / pow(2, i + 1) + 1; p++) {
				firfilter_fft[log_times - i - 1][p][0] = fftout[log_times - i - 1][p][0];
				firfilter_fft[log_times - i - 1][p][1] = fftout[log_times - i - 1][p][1];
			}
		}
	}

	double gain = 1;
	std::vector<uint32_t> nowfft_size;
	std::vector<uint32_t> zero_size;
	std::vector<uint32_t> pudding_size;
	std::vector<uint32_t> realfft_size;
	std::vector<uint32_t> add_size;
	std::vector<FFTWPlan> fft;
	std::vector<FFTWPlan> ifft;
	std::vector<FFTWDoubleArray> fftin;
	std::vector<FFTWComplexArray> fftout;
	std::vector<FFTWDoubleArray> ifftout;
	std::vector<FFTWComplexArray> ifftin;
	std::vector<FFTWDoubleArray> prebuffer;
	std::vector<FFTWComplexArray> firfilter_fft;

};

class Pcm2DsdSampleWriter::Pcm2DsdSampleWriterImpl {
public:
	explicit Pcm2DsdSampleWriterImpl(DsdTimes dsd_times) {
		dsd_times_ = static_cast<uint32_t>(dsd_times);
		logger_ = LoggerManager::GetInstance().GetLogger("Pcm2DsdConverter");
		FIRFilter();
		NoiseShapingCoeff();
		FFTW_LIB.fftw_make_planner_thread_safe();
	}

	void Init(uint32_t input_sample_rate) {
		auto dsd_times = pow(2, dsd_times_);

		if (input_sample_rate % 44100 == 0) {
			times_ = dsd_times / (input_sample_rate / 44100);
			dsd_sampling_rate_ = input_sample_rate * times_;
		}
		else {
			times_ = dsd_times / (input_sample_rate / 48000);
			dsd_sampling_rate_ = input_sample_rate * times_;
		}

		section_1_ = FIRFilter().size();
		order_ = NoiseShapingCoeff().GetLength();
		log_times_ = static_cast<uint32_t>(log(times_) / log(2));
		fft_size_ = (section_1_ + 1) * times_;
		data_size_ = fft_size_ / 2;
		dsd_times_ = times_;

		XAMP_LOG_D(logger_, "DSD Times: {} Times: {} DsdSampleRate: {} section_1: {} logtimes: {} fftsize: {}", 
			dsd_times,
			times_, 
			dsd_sampling_rate_,
			section_1_,
			log_times_, 
			fft_size_);

		lch_ctx_.Init(log_times_, times_, fft_size_, section_1_, logger_);
		rch_ctx_.Init(log_times_, times_, fft_size_, section_1_, logger_);		
	}

	uint32_t GetDataSize() const {
		return data_size_;
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
		XAMP_ASSERT(num_samples % 2 == 0);
		for (auto i = 0; i < num_samples / AudioFormat::kMaxChannel; ++i) {
			lch_src_[i] = samples[i * 2 + 0];
			rch_src_[i] = samples[i * 2 + 1];
		}
	}

	void ProcessChannel(const std::vector<double> &channel, FFTWContext & ctx, std::fstream &output_file) {
		std::vector<double> delta_buffer;
		std::vector<double> buffer;
		std::vector<int8_t> out;

		delta_buffer.resize(order_ + 1);
		buffer.resize(fft_size_);
		out.resize(data_size_);

		split_num_ = (channel.size() / data_size_) * dsd_times_;
		auto data = reinterpret_cast<const uint8_t*>(channel.data());

		XAMP_ASSERT(channel.size() % data_size_ == 0);

		const auto& NS = NoiseShapingCoeff();
		double deltagain = 0.5;
		deltagain = ctx.gain * deltagain;
		double error_y = 0;
		double x_in = 0;
		auto i = 0u;
		auto t = 0u;
		auto q = 0u;
		auto p = 0u;

		XAMP_LOG_D(logger_, "Split count: {}, data size: {}", split_num_, data_size_);

		for (i = 0u; i < split_num_; ++i) {
			MemoryCopy(buffer.data(), data, 8 * (data_size_ / dsd_times_));

			for (t = 0; t < log_times_; t++) {
				q = 0;
				for (p = 0; p < ctx.zero_size[t]; p++) {
					ctx.fftin[t][q] = buffer[p];
					q++;
					ctx.fftin[t][q] = 0;
					q++;
				}

				MemorySet(ctx.fftin[t].get() + q, 0, 8 * (ctx.nowfft_size[t] - q));
				FFTW_LIB.fftw_execute(ctx.fft[t].get());

				for (p = 0; p < ctx.realfft_size[t]; p++) {
					ctx.ifftin[t][p][0] = ctx.fftout[t][p][0] * ctx.firfilter_fft[t][p][0] - ctx.fftout[t][p][1] * ctx.firfilter_fft[t][p][1];
					ctx.ifftin[t][p][1] = ctx.fftout[t][p][0] * ctx.firfilter_fft[t][p][1] + ctx.firfilter_fft[t][p][0] * ctx.fftout[t][p][1];
				}
				FFTW_LIB.fftw_execute(ctx.ifft[t].get());

				for (p = 0; p < ctx.pudding_size[t]; p++) {
					buffer[p] = ctx.prebuffer[t][p] + ctx.ifftout[t][p];
				}

				q = 0;
				for (p = ctx.pudding_size[t]; p < ctx.nowfft_size[t]; p++) {
					buffer[p] = ctx.prebuffer[t][q] = ctx.ifftout[t][p];
					q++;
				}
			}

			for (q = 0u; q < data_size_; q++) {
				x_in = buffer[q] * deltagain;

				for (t = 0u; t < order_; t++) {
					x_in += NS.Get(0, t) * delta_buffer[t];
				}

				if (x_in >= 0.0) {
					out[q] = 1;
					error_y = -1.0;
				}
				else {
					out[q] = 0;
					error_y = 1.0;
				}
				for (t = order_; t > 0; t--) {
					delta_buffer[t] = delta_buffer[t - 1];
				}

				delta_buffer[0] = x_in + error_y;

				for (t = 0u; t < order_; t++) {
					delta_buffer[0] += NS.Get(1, t) * delta_buffer[t + 1];
				}
			}

			output_file.write(reinterpret_cast<const char*>(out.data()), data_size_);
			if (!output_file) {
				throw PlatformSpecException();
			}
			data += 8 * (data_size_ / dsd_times_);
		}
	}

	void CreateTempFile() {
		const auto temp_path = Fs::temp_directory_path();
		lch_out_path_ = temp_path / Fs::path(MakeTempFileName() + ".tmp");
		lch_out_.open(lch_out_path_.native(), std::ios::out | std::ios::trunc | std::ios::binary);
		if (!lch_out_.is_open()) {
			throw PlatformSpecException();
		}

		rch_out_path_ = temp_path / Fs::path(MakeTempFileName() + ".tmp");
		rch_out_.open(rch_out_path_.native(), std::ios::out | std::ios::trunc | std::ios::binary);
		if (!rch_out_.is_open()) {
			throw PlatformSpecException();
		}
	}

	void RemoveTempFile() {
		lch_out_.close();
		rch_out_.close();
		Fs::remove(lch_out_path_);
		Fs::remove(rch_out_path_);
	}

	bool MargeChannel(AudioBuffer<int8_t>& buffer) {
		XAMP_LOG_D(logger_, "Marge L/R Channel");

		constexpr std::array<uint8_t, 2> kDoPMarker { 0x05, 0xFA };

		lch_out_.close();
		rch_out_.close();

		lch_out_.open(lch_out_path_.native(), std::ios::in | std::ios::binary);
		if (!lch_out_.is_open()) {
			throw PlatformSpecException();
		}

		rch_out_.open(rch_out_path_.native(), std::ios::in | std::ios::binary);
		if (!rch_out_.is_open()) {
			throw PlatformSpecException();
		}

		std::vector<uint8_t> tmpdataL(data_size_);
		std::vector<uint8_t> tmpdataR(data_size_);
		std::vector<float> dop_data(data_size_ * 2);
		
		for (auto i = 0u; i < split_num_; i++) {
		 	lch_out_.read(reinterpret_cast<char*>(tmpdataL.data()), tmpdataL.size());
			if (!lch_out_) {
				throw PlatformSpecException();
			}

			rch_out_.read(reinterpret_cast<char*>(tmpdataR.data()), tmpdataR.size());
			if (!rch_out_) {
				throw PlatformSpecException();
			}

			auto p = 0u;
			auto c = 0u;
			auto o = 0u;
			auto k = 0u;

			std::vector<uint8_t> onebit(data_size_ / 4);

			for (k = 0u; k < data_size_ / 4; k++) {
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

			// DOP format:
			// 0x05 L1 0x05 R1, 0xFA L2 0xFA R2, 0x05 L3 0x05 R3

			for (auto sample = 0u; sample < onebit.size() / 2; sample++) {
				Int24 val;
				val.data[0] = onebit[sample * 2 + 0];
				val.data[1] = onebit[sample * 2 + 1];
				val.data[2] = kDoPMarker[c];
				auto v = val.To32Int();
				XAMP_ASSERT(v != 0);
				dop_data[o] = static_cast<float>(v) / kFloat24Scale;
				XAMP_ASSERT(dop_data[o] != 0);
				++o;
				if (o % 2 == 0) {
					c = (c + 1) % kDoPMarker.size();
				}
			}
			BufferOverFlowThrow(buffer.TryWrite(reinterpret_cast<int8_t*>(dop_data.data()), dop_data.size() * 4));
		}
		RemoveTempFile();
		return true;
	}

	bool Process(float const* samples, size_t num_samples, AudioBuffer<int8_t>& buffer) {
		try {
			CreateTempFile();
		}
		catch (std::exception const& e) {
			XAMP_LOG_D(logger_, e.what());
			RemoveTempFile();
			return true;
		}

		SplitChannel(samples, num_samples);

		const auto lch_task= GetDSPThreadPool().Spawn([this](auto index) {
			ProcessChannel(lch_src_, lch_ctx_, lch_out_);
			});
		const auto rch_task = GetDSPThreadPool().Spawn([this](auto index) {
			ProcessChannel(rch_src_, rch_ctx_, rch_out_);
			});

		try {
			lch_task.wait();
			rch_task.wait();
			return MargeChannel(buffer);
		}
		catch (std::exception const &e) {
			XAMP_LOG_D(logger_, e.what());
			RemoveTempFile();
			return true;
		}
	}

	uint32_t order_;
	uint32_t dsd_sampling_rate_{ 0 };
	uint32_t section_1_{ 0 };
	uint32_t data_size_{ 0 };
	uint32_t times_{ 0 };
	uint32_t dsd_times_{ 0 };
	uint32_t log_times_{ 0 };
	uint32_t fft_size_{ 0 };
	uint32_t split_num_{ 0 };
	FastMutex mutex_;
	Path lch_out_path_;
	Path rch_out_path_;
	FFTWContext lch_ctx_;
	FFTWContext rch_ctx_;
	std::fstream lch_out_;
	std::fstream rch_out_;
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

uint32_t Pcm2DsdSampleWriter::GetDataSize() const {
	return impl_->GetDataSize();
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
