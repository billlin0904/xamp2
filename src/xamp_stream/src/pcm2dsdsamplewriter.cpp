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
#include <base/stl.h>
#include <base/stopwatch.h>

#include <stream/fftwlib.h>
#include <stream/dsd_utils.h>
#include <stream/pcm2dsdsamplewriter.h>

namespace xamp::stream {

#if defined(XAMP_OS_WIN) || defined(XAMP_OS_MAC)

static uint8_t ReverseBits(uint8_t num) {
	auto NO_OF_BITS = sizeof(num) * 8;
	auto reverse_num = 0;

	for (auto i = 0; i < NO_OF_BITS; i++) {
		if ((num & (1 << i)))
			reverse_num |= 1 << ((NO_OF_BITS - 1) - i);
	}
	return reverse_num;
}

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
	Vector<double> data_;
};

static const Vector<double> & FIRFilter() {
	static constexpr auto read_fir_filter = []() {
		Vector<double> data;

        std::ifstream file("FIRFilter.dat");
		if (file.fail()) {
			throw FileNotFoundException("Not found file 'FIRFilter.dat'");
		}
		char* end;
		std::string str;
		std::getline(file, str);
		auto filter_size = std::strtod(str.c_str(), &end);
		data.reserve(filter_size);

		while (std::getline(file, str)) {
			data.push_back(std::strtod(str.c_str(), &end));
		}
		return data;
	};

	static const auto lut = read_fir_filter();
	return lut;
}

static const Double2DArray& NoiseShapingCoeff() {
	static constexpr auto read_noise_shaping_coeff = []() {
		Double2DArray data;

        std::ifstream file("NoiseShapingCoeff.dat");
		if (file.fail()) {
			throw FileNotFoundException("Not found file 'NoiseShapingCoeff.dat'");
		}

		std::string str;
		auto i = 0; int s = 0;
		char* end;

		std::getline(file, str);

		auto order = std::strtod(str.c_str(), &end);
		data.Resize(order, 2);

		while (std::getline(file, str)) {
			if (str != "0") {
				if (i == 0)
					data.Set(i, s, std::strtod(str.c_str(), &end));
				else {
					data.Set(i, order - s - 1, std::strtod(str.c_str(), &end));
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

	static const auto lut = read_noise_shaping_coeff();
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
				"Make FFT/IFFT I/O {} (log_times:{} p:{} i:{}) Buffer",
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
	Vector<uint32_t> nowfft_size;
	Vector<uint32_t> zero_size;
	Vector<uint32_t> pudding_size;
	Vector<uint32_t> realfft_size;
	Vector<uint32_t> add_size;
	Vector<FFTWPlan> fft;
	Vector<FFTWPlan> ifft;
	Vector<FFTWDoubleArray> fftin;
	Vector<FFTWComplexArray> fftout;
	Vector<FFTWDoubleArray> ifftout;
	Vector<FFTWComplexArray> ifftin;
	Vector<FFTWDoubleArray> prebuffer;
	Vector<FFTWComplexArray> firfilter_fft;
};

#define NO_TEST_DSD_FILE 1

class Pcm2DsdSampleWriter::Pcm2DsdSampleWriterImpl {
public:
	explicit Pcm2DsdSampleWriterImpl(DsdTimes dsd_times) {
		dsd_times_ = static_cast<uint32_t>(dsd_times);
		logger_ = LoggerManager::GetInstance().GetLogger(kPcm2DsdConverterLoggerName);
	}

	~Pcm2DsdSampleWriterImpl() {
		RemoveTempFile();
		if (tp_ != nullptr) {
			tp_->Stop();
		}
	}

	void Init(uint32_t input_sample_rate, CpuAffinity affinity, Pcm2DsdConvertModes convert_mode) {
		FIRFilter();
		NoiseShapingCoeff();

		convert_mode_ = convert_mode;

		constexpr auto kMaxPcm2DsdThread = 4;
		tp_ = MakeThreadPool(kDSPThreadPoolLoggerName,
			ThreadPriority::NORMAL,
			kMaxPcm2DsdThread,
			affinity);

		auto dsd_times = pow(2, dsd_times_);

		if (input_sample_rate % kPcmSampleRate441 == 0) {
			times_ = dsd_times / (input_sample_rate / kPcmSampleRate441);
			dsd_sampling_rate_ = input_sample_rate * times_;
		}
		else {
			times_ = dsd_times / (input_sample_rate / kPcmSampleRate48);
			dsd_sampling_rate_ = input_sample_rate * times_;
		}

		section_1_ = FIRFilter().size();
		order_ = NoiseShapingCoeff().GetLength();
		log_times_ = static_cast<uint32_t>(log(times_) / log(2));
		fft_size_ = (section_1_ + 1) * times_;
		data_size_ = fft_size_ / 2;
		dsd_times_ = times_;

		XAMP_LOG_D(logger_, "Type: {} DSD Times: {} Times: {} DsdSampleRate: {} section_1: {} logtimes: {} fftsize: {}",
			convert_mode_,
			dsd_times,
			times_, 
			dsd_sampling_rate_,
			section_1_,
			log_times_, 
			fft_size_);

		lch_ctx_.Init(log_times_, times_, fft_size_, section_1_, logger_);
		rch_ctx_.Init(log_times_, times_, fft_size_, section_1_, logger_);

#ifndef NO_TEST_DSD_FILE
		lch_out_.open(R"(C:\Users\rdbill0452\Documents\Github\xamp2\src\xamp\test_dsd\Catch You Catch Me_tmpLDSD)", std::ios::in | std::ios::binary);
		lch_out_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		lch_out_.seekg(12873984);

		rch_out_.open(R"(C:\Users\rdbill0452\Documents\Github\xamp2\src\xamp\test_dsd\Catch You Catch Me_tmpRDSD)", std::ios::in | std::ios::binary);
		rch_out_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		rch_out_.seekg(12873984);
#endif
	}

	uint32_t GetDataSize() const {
		return data_size_;
	}

	uint32_t GetDsdSampleRate() const {
		return dsd_sampling_rate_;
	}

	uint32_t GetDsdSpeed() const {
		if (dsd_sampling_rate_ % kPcmSampleRate441 == 0) {
			return dsd_sampling_rate_ / kPcmSampleRate441;
		}
		return dsd_sampling_rate_ / kPcmSampleRate48;
	}

	void SplitChannel(float const* samples, size_t num_samples) {
		XAMP_ASSERT(num_samples % 2 == 0);
		auto channel_size = num_samples / AudioFormat::kMaxChannel;

		if (lch_src_.size() != data_size_) {
			lch_src_.resize(data_size_);
		}
		if (rch_src_.size() != data_size_) {
			rch_src_.resize(data_size_);
		}

		for (auto i = 0; i < data_size_; ++i) {
			if (i >= channel_size) {
				lch_src_[i] = 0;
				rch_src_[i] = 0;
			} else {
				lch_src_[i] = samples[i * 2 + 0];
				rch_src_[i] = samples[i * 2 + 1];
			}
			XAMP_ASSERT(lch_src_[i] >= -1.0 && lch_src_[i] <= 1.0);
			XAMP_ASSERT(rch_src_[i] >= -1.0 && rch_src_[i] <= 1.0);
			// todo: Silent signal test.
			//lch_src_[i] = 0;
			//rch_src_[i] = 0;
		}
	}

	void ProcessChannel(const Vector<double> &channel, FFTWContext & ctx, std::fstream & output_file) {
		Vector<double> delta_buffer;
		Vector<double> buffer;
		Vector<int8_t> out;

		delta_buffer.resize(order_ + 1);
		buffer.resize(fft_size_);
		out.resize(data_size_);

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

		XAMP_LOG_D(logger_, "Split count:{}, data size:{}", split_num_, data_size_);

		for (i = 0u; i < split_num_; ++i) {
			MemoryCopy(buffer.data(), data, 8 * (data_size_ / dsd_times_));

			for (t = 0u; t < log_times_; t++) {
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
			data += 8 * (data_size_ / dsd_times_);
		}
	}

	void CreateTempFile() {
		lch_out_path_ = GetTempFilePath();
		lch_out_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		lch_out_.open(lch_out_path_.native(), std::ios::out | std::ios::trunc | std::ios::binary);
		if (!lch_out_.is_open()) {
			throw PlatformSpecException();
		}

		rch_out_path_ = GetTempFilePath();
		rch_out_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		rch_out_.open(rch_out_path_.native(), std::ios::out | std::ios::trunc | std::ios::binary);
		if (!rch_out_.is_open()) {
			throw PlatformSpecException();
		}
	}

	void CloseAndRemoveFile(std::fstream& file, const Path &path) const {
		try {
			file.close();
			XAMP_LOG_D(logger_, String::Format("Close {} file.", path.filename()));
		}
		catch (std::exception const& e) {
			XAMP_LOG_E(logger_, String::Format("Close {} file failure! error: {}", path.filename(), e.what()));
		}

		try {
			Fs::remove(path);
			XAMP_LOG_D(logger_, String::Format("Remove {} file.", path.filename()));
		}
		catch (std::exception const& e) {
			XAMP_LOG_E(logger_, String::Format("Remove {} file failure! error: {}", path.filename(), e.what()));
		}
	}

	void RemoveTempFile() {
		CloseAndRemoveFile(lch_out_, lch_out_path_);
		CloseAndRemoveFile(rch_out_, rch_out_path_);
	}

	bool MargeChannel(AudioBuffer<int8_t>& buffer) {
		XAMP_LOG_D(logger_, "Marge L/R Channel");

		constexpr std::array<uint8_t, 2> kDoPMarker { 0x05, 0xFA };

#ifdef NO_TEST_DSD_FILE
		lch_out_.close();
		rch_out_.close();

		lch_out_.open(lch_out_path_.native(), std::ios::in | std::ios::binary);
		lch_out_.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		rch_out_.open(rch_out_path_.native(), std::ios::in | std::ios::binary);
		rch_out_.exceptions(std::ifstream::failbit | std::ifstream::badbit);
#endif
		
		Vector<uint8_t> tmpdataL(data_size_);
		Vector<uint8_t> tmpdataR(data_size_);
		Vector<uint8_t> onebit(data_size_ / 4);
		Vector<int32_t> debug_dop_data(onebit.size() / 2);
		Vector<float> dop_data(onebit.size() / 2);
		
		for (auto i = 0u; i < split_num_; i++) {
		 	lch_out_.read(reinterpret_cast<char*>(tmpdataL.data()), tmpdataL.size());
			rch_out_.read(reinterpret_cast<char*>(tmpdataR.data()), tmpdataR.size());

			auto p = 0u;
			auto c = 0u;
			auto o = 0u;
			auto k = 0u;

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

			if (convert_mode_ == Pcm2DsdConvertModes::PCM2DSD_DSD_DOP) {
				// DOP format:
				// 0x05 L1 0x05 R1, 0xFA L2 0xFA R2, 0x05 L3 0x05 R3 ...

				auto m = 0;

				for (auto sample = 0u; sample < onebit.size() / 2; sample += 2) {
					Int24 left;
					left.data[0] = onebit[sample * 2 + 2];
					left.data[1] = onebit[sample * 2 + 0];
					left.data[2] = kDoPMarker[c];
					auto v = left.To32Int();

					debug_dop_data[o] = v;
					if (kDoPMarker[c] == 0xFA) {
						v |= 0xFF000000u;
					}
					XAMP_ASSERT(v != 0);
					dop_data[o] = static_cast<float>(v) / 8388608;
					++o;

					Int24 right;
					right.data[0] = onebit[sample * 2 + 3];
					right.data[1] = onebit[sample * 2 + 1];
					right.data[2] = kDoPMarker[c];
					v = right.To32Int();

					debug_dop_data[o] = v;
					if (kDoPMarker[c] == 0xFA) {
						v |= 0xFF000000u;
					}
					XAMP_ASSERT(v != 0);
					dop_data[o] = static_cast<float>(v) / 8388608;
					++o;

					if (m % 2 == 0) {
						c = (c + 1) % kDoPMarker.size();
					}

					m += 2;
				}
                XAMP_LOG_D(logger_, "Write DOP data size:{} write:{}", dop_data.size() * 4, buffer.GetAvailableWrite());
				BufferOverFlowThrow(buffer.TryWrite(reinterpret_cast<int8_t*>(dop_data.data()), dop_data.size() * 4));
			} else {
				BufferOverFlowThrow(buffer.TryWrite(reinterpret_cast<int8_t*>(onebit.data()), onebit.size()));
			}
		}
#ifdef NO_TEST_DSD_FILE
		RemoveTempFile();
#endif
		return true;
	}

	bool Process(float const* samples, size_t num_samples, AudioBuffer<int8_t>& buffer) {
#ifdef NO_TEST_DSD_FILE
		try {
			CreateTempFile();
		}
		catch (std::exception const& e) {
			XAMP_LOG_D(logger_, e.what());
			RemoveTempFile();
			return true;
		}

		SplitChannel(samples, num_samples);
#endif

		auto channel_size = num_samples / AudioFormat::kMaxChannel;
		split_num_ = (channel_size / data_size_) * dsd_times_;

#ifdef NO_TEST_DSD_FILE
		const auto lch_task= tp_->Spawn([this](auto index) {
			ProcessChannel(lch_src_, lch_ctx_, lch_out_);
			});
		const auto rch_task = tp_->Spawn([this](auto index) {
			ProcessChannel(rch_src_, rch_ctx_, rch_out_);
			});

		lch_task.wait();
		rch_task.wait();
#endif
		return MargeChannel(buffer);
	}

	uint32_t order_{ 0 };
	uint32_t dsd_sampling_rate_{ 0 };
	uint32_t section_1_{ 0 };
	uint32_t data_size_{ 0 };
	uint32_t times_{ 0 };
	uint32_t dsd_times_{ 0 };
	uint32_t log_times_{ 0 };
	uint32_t fft_size_{ 0 };
	uint32_t split_num_{ 0 };
	Pcm2DsdConvertModes convert_mode_{ Pcm2DsdConvertModes::PCM2DSD_DSD_DOP };
	Path lch_out_path_;
	Path rch_out_path_;
	FFTWContext lch_ctx_;
	FFTWContext rch_ctx_;
	std::fstream lch_out_;
	std::fstream rch_out_;
	Vector<double> lch_src_;
	Vector<double> rch_src_;
	std::shared_ptr<Logger> logger_;
	AlignPtr<IThreadPool> tp_;
};

XAMP_PIMPL_IMPL(Pcm2DsdSampleWriter)

Pcm2DsdSampleWriter::Pcm2DsdSampleWriter(DsdTimes dsd_times)
	: impl_(MakeAlign<Pcm2DsdSampleWriterImpl>(dsd_times)) {
}

void Pcm2DsdSampleWriter::Init(uint32_t output_sample_rate, CpuAffinity affinity, Pcm2DsdConvertModes convert_mode) {
	impl_->Init(output_sample_rate, affinity, convert_mode);
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

class Pcm2DsdSampleWriter::Pcm2DsdSampleWriterImpl {
public:
	Pcm2DsdSampleWriterImpl(DsdTimes dsd_times) {
    }
};

XAMP_PIMPL_IMPL(Pcm2DsdSampleWriter)

Pcm2DsdSampleWriter::Pcm2DsdSampleWriter(DsdTimes dsd_times) {
}

uint32_t Pcm2DsdSampleWriter::GetDataSize() const {
	return 0;
}

uint32_t Pcm2DsdSampleWriter::GetDsdSampleRate() const {
	return 0;
}

uint32_t Pcm2DsdSampleWriter::GetDsdSpeed() const {
	return 0;
}

void Pcm2DsdSampleWriter::Init(uint32_t input_sample_rate, CpuAffinity affinity, Pcm2DsdConvertModes convert_mode) {
}

[[nodiscard]] bool Pcm2DsdSampleWriter::Process(BufferRef<float> const& input, AudioBuffer<int8_t>& buffer) {
    return Process(input.data(), input.size(), buffer);
}

bool Pcm2DsdSampleWriter::Process(float const* samples, size_t num_samples, AudioBuffer<int8_t>& buffer) {
    return false;
}

#endif

}
