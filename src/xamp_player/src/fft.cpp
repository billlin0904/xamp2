#include <base/enum.h>
#include <base/buffer.h>

#include <player/fft.h>

#ifdef XAMP_OS_WIN
#include <base/dll.h>
#include <fftw3.h>
#else
#include <base/unique_handle.h>
#include <Accelerate/Accelerate.h>
#endif

namespace xamp::player {

static size_t ComplexSize(size_t size) {
	return (size / 2) + 1;
}

#ifdef XAMP_OS_WIN
class FFTWLib {
public:
	FFTWLib()
		: module_(LoadModule("libfftw3f-3.dll"))
		, XAMP_LOAD_DLL_API(fftwf_destroy_plan)
		, XAMP_LOAD_DLL_API(fftwf_malloc)
		, XAMP_LOAD_DLL_API(fftwf_free)
		, XAMP_LOAD_DLL_API(fftwf_plan_guru_split_dft_c2r)
		, XAMP_LOAD_DLL_API(fftwf_plan_guru_split_dft_r2c)
		, XAMP_LOAD_DLL_API(fftwf_execute_split_dft_r2c)
		, XAMP_LOAD_DLL_API(fftwf_execute_split_dft_c2r) {
	}

private:
	ModuleHandle module_;

public:
	XAMP_DECLARE_DLL(fftwf_destroy_plan) fftwf_destroy_plan;
	XAMP_DECLARE_DLL(fftwf_malloc) fftwf_malloc;
	XAMP_DECLARE_DLL(fftwf_free) fftwf_free;
	XAMP_DECLARE_DLL(fftwf_plan_guru_split_dft_c2r) fftwf_plan_guru_split_dft_c2r;
	XAMP_DECLARE_DLL(fftwf_plan_guru_split_dft_r2c) fftwf_plan_guru_split_dft_r2c;
	XAMP_DECLARE_DLL(fftwf_execute_split_dft_r2c) fftwf_execute_split_dft_r2c;
	XAMP_DECLARE_DLL(fftwf_execute_split_dft_c2r) fftwf_execute_split_dft_c2r;
};

#define FFTW Singleton<FFTWLib>::GetInstance()

struct FFTWFloatPtrTraits final {
	template <typename T>
	void operator()(T value) const {
		FFTW.fftwf_free(value);
	}
};

using FFTWPtr = std::unique_ptr<float, FFTWFloatPtrTraits>;

static FFTWPtr MakeFFTWBuffer(size_t size) {
	return FFTWPtr(static_cast<float*>(FFTW.fftwf_malloc(sizeof(float) * size)));
}

class Window::WindowImpl {
public:
	WindowImpl() = default;

	void Init(size_t size, WindowType type = WindowType::HAMMING) {
		window_ = MakeFFTWBuffer(size);
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
		data_ = MakeFFTWBuffer(size);
		re_ = MakeFFTWBuffer(complex_size_);
		im_ = MakeFFTWBuffer(complex_size_);
		output_ = ComplexValarray(Complex(), complex_size_);

		fftw_iodim dim;
		dim.n = static_cast<int>(size);
		dim.is = 1;
		dim.os = 1;
		forward_.reset(FFTW.fftwf_plan_guru_split_dft_r2c(1,
			&dim,
			0,
			nullptr,
			data_.get(),
			re_.get(),
			im_.get(),
			FFTW_ESTIMATE));

		backward_.reset(FFTW.fftwf_plan_guru_split_dft_c2r(1,
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

		FFTW.fftwf_execute_split_dft_r2c(forward_.get(),
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

	struct FFTWPlanTraits final {
		static fftwf_plan invalid() {
			return nullptr;
		}

		static void close(fftwf_plan value) {
			FFTW.fftwf_destroy_plan(value);
		}
	};

	using FFTWPlan = UniqueHandle<fftwf_plan, FFTWPlanTraits>;

	size_t complex_size_{ 0 };
	FFTWPtr data_;
	FFTWPtr re_;
	FFTWPtr im_;
	Window window_;
	ComplexValarray output_;
	FFTWPlan forward_;
	FFTWPlan backward_;
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
		window_.Init(size);
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
		output_ = ComplexValarray(Complex(), complex_size_); (Complex(), complex_size_);
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
	Window window_;
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

void FFT::LoadFFTLib() {
#ifdef XAMP_OS_WIN
	Singleton<FFTWLib>::GetInstance();
#endif
}

}
