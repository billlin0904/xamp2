#include <base/enum.h>
#include <base/buffer.h>

#include <stream/fft.h>

#ifdef XAMP_OS_WIN
#include <base/dll.h>
#include <stream/fftwlib.h>
#else
#include <base/unique_handle.h>
#include <Accelerate/Accelerate.h>
#endif

namespace xamp::stream {

static size_t ComplexSize(size_t size) {
	return (size / 2) + 1;
}

#ifdef XAMP_OS_WIN

using FFTWPtr = std::unique_ptr<float[], FFTWPtrTraits<float>>;

class Window::WindowImpl {
public:
	WindowImpl() = default;

	void Init(size_t size, WindowType type = WindowType::HAMMING) {
		window_ = MakeFFTWBuffer<float>(size);
		for (auto i = 0; i < size; ++i) {
			window_.get()[i] = operator()(i, size);
		}
	}

	void operator()(float const* samples, float* buffer, size_t size) const {
		for (size_t i = 0; i < size; ++i) {
			buffer[i] = samples[i] * window_.get()[i];
		}
	}

	float operator()(size_t i, size_t N) const {
		return 0.54 - 0.46 * std::cos((2.0 * XAMP_PI * i) / (N - 1));
	}
protected:
	FFTWPtr window_;
};

class FFT::FFTImpl {
public:
	FFTImpl() = default;

	void Init(size_t size) {
		window_.Init(size);
		complex_size_ = ComplexSize(size);
		data_ = MakeFFTWBuffer<float>(size);
		re_ = MakeFFTWBuffer<float>(complex_size_);
		im_ = MakeFFTWBuffer<float>(complex_size_);
		output_ = ComplexValarray(Complex(), complex_size_);

		fftw_iodim dim;
		dim.n = static_cast<int>(size);
		dim.is = 1;
		dim.os = 1;
		forward_.reset(FFTWF_LIB.fftwf_plan_guru_split_dft_r2c(1,
			&dim,
			0,
			nullptr,
			data_.get(),
			re_.get(),
			im_.get(),
			FFTW_ESTIMATE));

		backward_.reset(FFTWF_LIB.fftwf_plan_guru_split_dft_c2r(1,
			&dim,
			0,
			nullptr,
			re_.get(),
			im_.get(),
			data_.get(),
			FFTW_ESTIMATE));
	}

	const ComplexValarray& Forward(float const* signals, size_t size) {
		MemoryCopy(data_.get(), signals, sizeof(float) * size);

		FFTWF_LIB.fftwf_execute_split_dft_r2c(forward_.get(),
			data_.get(),
			re_.get(),
			im_.get());

		auto const re = re_.get();
		auto const im = im_.get();

		for (size_t i = 0; i < complex_size_; ++i) {
			output_[i] = Complex(re[i], im[i]);
		}

		return output_;
	}

	size_t complex_size_{ 0 };
	FFTWPtr data_;
	FFTWPtr re_;
	FFTWPtr im_;
	Window window_;
	ComplexValarray output_;
	FFTWFPlan forward_;
	FFTWFPlan backward_;
};

#else

class Window::WindowImpl {
public:
	WindowImpl() = default;

	void Init(size_t size, WindowType type = WindowType::HAMMING) {
		window_ = MakeAlignedArray<float>(size);
		(void)type;
		::vDSP_hamm_window(window_.get(), size, vDSP_HANN_DENORM);
	}

	void operator()(float const* samples, float* buffer, size_t size) const {
		::vDSP_vmul(samples, 1, window_.get(), 1, buffer, 1, size);
	}

	float operator()(size_t i, size_t N) const {
		return 0.54 - 0.46 * std::cos((2.0 * XAMP_PI * i) / (N - 1));
	}
protected:
	AlignArray<float> window_;
};

class FFT::FFTImpl {
public:
	FFTImpl() = default;

	void Init(size_t size) {
		size_ = size;
        size_over2_ = size_ / 2;
        log2n_size_ = std::log2(size);
		complex_size_ = log2n_size_;
		output_ = ComplexValarray(Complex(), complex_size_);
		input_ = MakeAlignedArray<float>(size);
		fft_setup_.reset(::vDSP_create_fftsetup(log2n_size_, FFT_RADIX2));
		re_ = MakeAlignedArray<float>(size_over2_);
		im_ = MakeAlignedArray<float>(size_over2_);
		split_complex_.realp = re_.get();
		split_complex_.imagp = im_.get();
	}

	const ComplexValarray& Forward(float const* signals, size_t size) {
		MemoryCopy(input_.get(), signals, sizeof(float) * size);

		::vDSP_ctoz(reinterpret_cast<const COMPLEX*>(input_.get()), 2, &split_complex_, 1, size_over2_);
		::vDSP_fft_zrip(fft_setup_.get(), &split_complex_, 1, log2n_size_, FFT_FORWARD);

		split_complex_.imagp[0] = 0.0;

		auto re = re_.get();
		auto im = im_.get();

		for (size_t i = 0; i < complex_size_; ++i) {
			output_[i] = Complex(re[i], im[i]);
		}

		return output_;
	}

private:
	struct FFTSetupTraits final {
		static FFTSetup invalid() {
			return nullptr;
		}

		static void close(FFTSetup value) {
			::vDSP_destroy_fftsetup(value);
		}
	};

	using FFTSetupHandle = UniqueHandle<FFTSetup, FFTSetupTraits>;

	size_t size_{ 0 };
	size_t log2n_size_{ 0 };
	size_t size_over2_{ 0 };
    size_t complex_size_{ 0 };
	FFTSetupHandle fft_setup_;
	DSPSplitComplex split_complex_;
	AlignArray<float> input_;
	AlignArray<float> re_;
	AlignArray<float> im_;
	ComplexValarray output_;
};

#endif

Window::Window()
	: impl_(MakeAlign<WindowImpl>()) {
}

XAMP_PIMPL_IMPL(Window)

void Window::Init(size_t size, WindowType type) {
	impl_->Init(size, type);
}

float Window::operator()(size_t i, size_t N) const {
	return impl_->operator()(i, N);
}

void Window::operator()(float const* samples, float* buffer, size_t size) const {
	return impl_->operator()(samples, buffer, size);
}

FFT::FFT()
	: impl_(MakeAlign<FFTImpl>()) {
}

XAMP_PIMPL_IMPL(FFT)

void FFT::Init(size_t size) {
	impl_->Init(size);
}

const ComplexValarray& FFT::Forward(float const* data, size_t size) {
	return impl_->Forward(data, size);
}

}
