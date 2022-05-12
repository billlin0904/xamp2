#include <base/enum.h>
#include <base/buffer.h>
#include <base/assert.h>
#include <base/math.h>

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

class Window::WindowImpl {
public:
	WindowImpl() = default;

	void Init(size_t frame_size, WindowType type = WindowType::HAMMING) {
		frame_size_ = frame_size;
		data_ = MakeBuffer<float>(frame_size);
		SetWindowType(type);
		for (size_t i = 0; i < frame_size; i++) {
			data_[i] = (*this.*dispatch_)(i, frame_size);
		}
	}

	void SetWindowType(WindowType type) {
		switch (type) {
		case WindowType::NO_WINDOW:
			dispatch_ = &WindowImpl::NoWindow;
			break;
		case WindowType::HAMMING:
			dispatch_ = &WindowImpl::HammingWindow;
			break;
		case WindowType::BLACKMAN_HARRIS:
		default:
			dispatch_ = &WindowImpl::BlackmanHarrisWindow;
			break;
		}
	}

	void operator()(float* buffer, size_t size) const noexcept {
		XAMP_ASSERT(frame_size_ == size);

		for (size_t i = 0; i < frame_size_; i++) {
			buffer[i] *= data_[i];
		}
	}
private:
	float NoWindow(size_t i, size_t N) {
		return 1;
	}

	float HammingWindow(size_t i, size_t N) {
		return 0.54 - 0.46 * std::cos((2.0 * XAMP_PI * i) / (N - 1));
	}

	float BlackmanHarrisWindow(size_t i, size_t N) {
		constexpr float a0 = 0.35875f;
		constexpr float a1 = 0.48829f;
		constexpr float a2 = 0.14128f;
		constexpr float a3 = 0.01168f;

		return a0 - (a1 * cosf((2.0f * XAMP_PI * i) / (N - 1)))
			+ (a2 * cosf((4.0f * XAMP_PI * i) / (N - 1)))
			- (a3 * cosf((6.0f * XAMP_PI * i) / (N - 1)));
	}

	typedef float (WindowImpl::* WindowDispatch)(size_t i, size_t N);
	size_t frame_size_{0};
	WindowDispatch dispatch_{nullptr};
	Buffer<float> data_;
};

#ifdef XAMP_OS_WIN

using FFTWPtr = std::unique_ptr<float[], FFTWPtrTraits<float>>;

class FFT::FFTImpl {
public:
	FFTImpl() = default;

	void Init(size_t frame_size) {
		XAMP_ASSERT(IsPowerOfTwo(frame_size));
		frame_size_ = frame_size;
		complex_size_ = ComplexSize(frame_size);
		data_ = MakeFFTWBuffer<float>(frame_size);
		re_ = MakeFFTWBuffer<float>(complex_size_);
		im_ = MakeFFTWBuffer<float>(complex_size_);
		output_ = ComplexValarray(Complex(), complex_size_);

		fftw_iodim dim;
		dim.n = static_cast<int>(frame_size);
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

	const ComplexValarray& Forward(float const* signals, size_t frame_size) {
		XAMP_ASSERT(frame_size_ == frame_size);

		MemoryCopy(data_.get(), signals, sizeof(float) * frame_size);

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

	size_t frame_size_{ 0 };
	size_t complex_size_{ 0 };
	FFTWPtr data_;
	FFTWPtr re_;
	FFTWPtr im_;
	ComplexValarray output_;
	FFTWFPlan forward_;
	FFTWFPlan backward_;
};

#else

class FFT::FFTImpl {
public:
	FFTImpl() = default;

	void Init(size_t frame_size) {
		XAMP_ASSERT(IsPowerOfTwo(frame_size));
		frame_size_ = frame_size;
        size_over2_ = frame_size_ / 2;
        log2n_size_ = std::log2(frame_size);
		complex_size_ = log2n_size_;
		output_ = ComplexValarray(Complex(), complex_size_);
		input_ = MakeAlignedArray<float>(frame_size);
		fft_setup_.reset(::vDSP_create_fftsetup(log2n_size_, FFT_RADIX2));
		re_ = MakeAlignedArray<float>(size_over2_);
		im_ = MakeAlignedArray<float>(size_over2_);
		split_complex_.realp = re_.get();
		split_complex_.imagp = im_.get();
	}

	const ComplexValarray& Forward(float const* signals, size_t frame_size) {
		MemoryCopy(input_.get(), signals, sizeof(float) * frame_size);

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

	size_t frame_size_{ 0 };
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

void Window::Init(size_t frame_size, WindowType type) {
	impl_->Init(frame_size, type);
}

void Window::SetWindowType(WindowType type) {
	impl_->SetWindowType(type);
}

void Window::operator()(float* buffer, size_t size) const noexcept {
	return impl_->operator()(buffer, size);
}

FFT::FFT()
	: impl_(MakeAlign<FFTImpl>()) {
}

XAMP_PIMPL_IMPL(FFT)

void FFT::Init(size_t frame_size) {
	impl_->Init(frame_size);
}

const ComplexValarray& FFT::Forward(float const* data, size_t size) {
	return impl_->Forward(data, size);
}

}
