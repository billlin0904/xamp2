#include <functional>

#include <base/memory.h>
#include <base/buffer.h>
#include <base/assert.h>
#include <base/math.h>
#include <base/logger_impl.h>
#include <base/exception.h>
#include <stream/fft.h>

#ifdef XAMP_OS_WIN
#include <base/dll.h>
#include <stream/fftwlib.h>
#else
#include <base/unique_handle.h>
#include <Accelerate/Accelerate.h>
#endif

XAMP_STREAM_NAMESPACE_BEGIN

namespace {
	size_t ComplexSize(size_t size) {
		return (size / 2) + 1;
	}
}

class Window::WindowImpl {
public:
	WindowImpl() = default;

	void Init(size_t frame_size, WindowType type) {
		frame_size_ = frame_size;
		data_ = MakeBuffer<float>(frame_size);
		cos_lut_ = MakeBuffer<float>(frame_size);
		for (size_t i = 0; i < frame_size; i++) {
			cos_lut_[i] = cosf((2.0 * XAMP_PI * i) / (frame_size - 1));
		}
		SetWindowType(type);
	}

	void SetWindowType(WindowType type) {
		switch (type) {
		case WindowType::NO_WINDOW:
			dispatch_ = bind_front(&WindowImpl::NoWindow, this);
			break;
		case WindowType::HAMMING:
			dispatch_ = bind_front(&WindowImpl::HammingWindow, this);
			break;
		case WindowType::HANN:
			dispatch_ = bind_front(&WindowImpl::HannWindow, this);
			break;
		case WindowType::BLACKMAN_HARRIS:
		default:
			dispatch_ = bind_front(&WindowImpl::BlackmanHarrisWindow, this);
			break;
		}
		for (size_t i = 0; i < frame_size_; i++) {
			data_[i] = std::invoke(dispatch_, i, frame_size_);
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

	float HannWindow(size_t i, size_t N) {
		return 0.5f * (1.0f - cos_lut_[i]);
	}

	float HammingWindow(size_t i, size_t N) {
		return 0.54f - 0.46f * cos_lut_[i];
	}

	float BlackmanHarrisWindow(size_t i, size_t N) {
		constexpr float a0 = 0.35875f;
		constexpr float a1 = 0.48829f;
		constexpr float a2 = 0.14128f;
		constexpr float a3 = 0.01168f;

		return a0 - (a1 * std::cos((2.0f * XAMP_PI * i) / (N - 1)))
			+ (a2 * std::cos((4.0f * XAMP_PI * i) / (N - 1)))
			- (a3 * std::cos((6.0f * XAMP_PI * i) / (N - 1)));
	}

	size_t frame_size_{0};
	Buffer<float> data_;
	Buffer<float> cos_lut_;
	std::function<float(size_t, size_t)> dispatch_;
};

#ifdef XAMP_OS_WIN

#if (USE_INTEL_MKL_LIB)

#define IfFailedThrowMKL(s) \
	if ((s) != 0 && !MKL_LIB.DftiErrorClass((s), DFTI_NO_ERROR)) { \
		throw LibraryException(MKL_LIB.DftiErrorMessage((s))); \
	}


struct DftiDescriptorTraits final {
	static DFTI_DESCRIPTOR_HANDLE invalid() {
		return nullptr;
	}

	static void close(DFTI_DESCRIPTOR_HANDLE value) {
		XAMP_EXPECTS(value != nullptr);
		MKL_LIB.DftiFreeDescriptor(&value);
	}
};

using DftiDescriptor = UniqueHandle<DFTI_DESCRIPTOR_HANDLE, DftiDescriptorTraits>;

class FFT::FFTImpl {
public:
	FFTImpl() = default;

	~FFTImpl() {
		XAMP_LOG_DEBUG("FFTImpl::~FFTImpl");
		descriptor_.reset();
	}

	void Init(size_t frame_size) {
		XAMP_EXPECTS(frame_size >= 2);

		descriptor_.reset();
		frame_size_ = frame_size;

		complex_size_ = ComplexSize(frame_size);
		DFTI_DESCRIPTOR_HANDLE descriptor = nullptr;
		MKL_LONG status = MKL_LIB.DftiCreateDescriptor_(&descriptor,
			DFTI_SINGLE,
			DFTI_REAL,
			1,
			static_cast<MKL_LONG>(frame_size_));
		IfFailedThrowMKL(status)
		descriptor_.reset(descriptor);

		status = MKL_LIB.DftiSetValue(descriptor_.get(),
			DFTI_PLACEMENT,
			DFTI_NOT_INPLACE);
		IfFailedThrowMKL(status)

		status = MKL_LIB.DftiSetValue(descriptor_.get(),
			DFTI_COMPLEX_STORAGE,
			DFTI_REAL_REAL);
		IfFailedThrowMKL(status)

		status = MKL_LIB.DftiCommitDescriptor(descriptor_.get());
		IfFailedThrowMKL(status)

		real_.resize(frame_size_);
		imag_.resize(frame_size_);
		output_.resize(complex_size_);
	}

	const ComplexValarray& Forward(const float* signals,
		std::size_t frame_size) {
		XAMP_ASSERT(frame_size_ == frame_size);
		XAMP_ASSERT(descriptor_);

		MKL_LONG status = MKL_LIB.DftiComputeForward(
			descriptor_.get(),
			const_cast<float*>(signals),
			real_.data(),
			imag_.data()
		);
		IfFailedThrowMKL(status)

		for (size_t i = 0; i < complex_size_; ++i) {
			output_[i] = Complex(real_[i], imag_[i]);
		}

		return output_;
	}

private:
	size_t complex_size_{ 0 };
	size_t frame_size_{ 0 };
	DftiDescriptor descriptor_;
	std::vector<float> real_;
	std::vector<float> imag_;
	ComplexValarray output_;
};
#else
class FFT::FFTImpl {
public:
	FFTImpl() = default;

	~FFTImpl() {
		XAMP_LOG_DEBUG("FFTImpl::~FFTImpl");
	}

	void Init(size_t frame_size) {
		XAMP_ASSERT(IsPowerOfTwo(frame_size));
		frame_size_ = frame_size;
		complex_size_ = ComplexSize(frame_size);
		data_ = MakeFFTWFBuffer(frame_size);
		re_ = MakeFFTWFBuffer(complex_size_);
		im_ = MakeFFTWFBuffer(complex_size_);
		output_ = ComplexValarray(Complex(), complex_size_);

		fftwf_iodim dim;
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
		if (!forward_) {
			throw LibraryException("Failed call fftwf_plan_guru_split_dft_r2c.");
		}

		backward_.reset(FFTWF_LIB.fftwf_plan_guru_split_dft_c2r(1,
			&dim,
			0,
			nullptr,
			re_.get(),
			im_.get(),
			data_.get(),
			FFTW_ESTIMATE));
		if (!backward_) {
			throw LibraryException("Failed call fftwf_plan_guru_split_dft_c2r.");
		}
	}

	const ComplexValarray& Forward(float const* signals, size_t frame_size) {
		XAMP_ASSERT(frame_size_ == frame_size);
		XAMP_ASSERT(forward_);

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
	FFTWFPtr data_;
	FFTWFPtr re_;
	FFTWFPtr im_;
	ComplexValarray output_;
	FFTWFPlan forward_;
	FFTWFPlan backward_;
};
#endif

#else

class FFT::FFTImpl {
public:
	FFTImpl() = default;

	void Init(size_t frame_size) {
		XAMP_ASSERT(IsPowerOfTwo(frame_size));
		frame_size_ = frame_size;
        size_over2_ = frame_size_ / 2;
        log2n_size_ = std::log2(frame_size);
        complex_size_ = ComplexSize(frame_size);
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
	ScopedArray<float> input_;
	ScopedArray<float> re_;
	ScopedArray<float> im_;
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

XAMP_STREAM_NAMESPACE_END
