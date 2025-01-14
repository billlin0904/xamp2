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
		SetWindowType(type);
		if (type == WindowType::HAMMING || type == WindowType::HANN) {
			cos_lut_.reserve(frame_size);
			for (size_t i = 0; i < frame_size; i++) {
				cos_lut_.push_back(std::cos((2.0 * XAMP_PI * i) / (frame_size - 1)));
			}
		}
		for (size_t i = 0; i < frame_size; i++) {
			data_[i] = std::invoke(dispatch_, i, frame_size);
		}
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

	std::function<float(size_t, size_t)> dispatch_;
	size_t frame_size_{0};
	Buffer<float> data_;
	std::vector<float> cos_lut_;
};

#ifdef XAMP_OS_WIN

#if (USE_INTEL_MKL_LIB)
#define CallDftiCreateDescriptor(desc,prec,domain,dim,sizes) \
    (/* single precision specific cases */ \
     ((prec)==DFTI_SINGLE && (dim)==1) ? \
		MKL_LIB.DftiCreateDescriptor_s_1d((desc),(domain),(sizes)) : \
     ((prec)==DFTI_SINGLE) ? \
		MKL_LIB.DftiCreateDescriptor_s_md((desc),(domain),(dim),(sizes)) : \
     /* double precision specific cases */ \
     ((prec)==DFTI_DOUBLE && (dim)==1) ? \
		MKL_LIB.DftiCreateDescriptor_d_1d((desc),(domain),(sizes)) : \
     ((prec)==DFTI_DOUBLE) ? \
		MKL_LIB.DftiCreateDescriptor_d_md((desc),(domain),(dim),(sizes)) : \
     /* no specific case matches, fall back to original call */ \
     MKL_LIB.DftiCreateDescriptor_((desc),(prec),(domain),(dim),(sizes)))

class FFT::FFTImpl {
public:
	FFTImpl()
		: descriptor_(nullptr)
		, frame_size_(0) {
	}

	~FFTImpl() {
		if (descriptor_) {
			MKL_LIB.DftiFreeDescriptor(&descriptor_);
			descriptor_ = nullptr;
		}
	}

	void Init(std::size_t frame_size) {
		if (descriptor_) {
			MKL_LIB.DftiFreeDescriptor(&descriptor_);
			descriptor_ = nullptr;
		}
		frame_size_ = frame_size;
		if (frame_size_ < 2) {
			throw std::runtime_error("FFT frame_size must be >= 2");
		}
		complex_size_ = ComplexSize(frame_size);
		MKL_LONG status = CallDftiCreateDescriptor(&descriptor_,
			DFTI_SINGLE,
			DFTI_REAL,
			1,
			static_cast<MKL_LONG>(frame_size_));
		if (status != DFTI_NO_ERROR) {
			throw std::runtime_error("DftiCreateDescriptor failed.");
		}

		status = MKL_LIB.DftiSetValue(descriptor_, DFTI_PLACEMENT, DFTI_NOT_INPLACE);
		if (status != DFTI_NO_ERROR) {
			throw std::runtime_error("DftiSetValue(PLACEMENT) failed.");
		}

		status = MKL_LIB.DftiCommitDescriptor(descriptor_);
		if (status != DFTI_NO_ERROR) {
			throw std::runtime_error("DftiCommitDescriptor failed.");
		}

		real_buffer_.resize(frame_size_, 0.f);
		cplx_buffer_.resize(frame_size_ / 2 + 1); // MKL¤º³¡¿é¥X?
	}

	const ComplexValarray& Forward(float const* data, std::size_t size) {
		if (!descriptor_) {
			throw std::runtime_error("FFT not initialized yet.");
		}
		if (size > frame_size_) {
			throw std::runtime_error("Input data size is bigger than frame_size_");
		}

		std::memcpy(real_buffer_.data(), data, sizeof(float) * size);

		MKL_LONG status =
			MKL_LIB.DftiComputeForward(descriptor_,
				real_buffer_.data(),
				cplx_buffer_.data());
		if (status != DFTI_NO_ERROR) {
			throw std::runtime_error("DftiComputeForward failed.");
		}

		for (size_t i = 0; i < complex_size_; ++i) {
			output_[i] = Complex(cplx_buffer_[i].real(), cplx_buffer_[i].imag());
		}

		return output_;
	}

private:
	size_t complex_size_;
	DFTI_DESCRIPTOR_HANDLE descriptor_;
	std::size_t frame_size_;
	std::vector<std::complex<float>> cplx_buffer_;
	std::vector<float> real_buffer_;
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
			FFTW_MEASURE));
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
			FFTW_MEASURE));
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
