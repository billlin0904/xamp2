#include <base/base.h>
#include <base/math.h>
#include <base/enum.h>

#define USE_FFTW

#ifdef XAMP_OS_WIN
	#ifdef USE_FFTW
	#include <base/dll.h>
	#include <base/singleton.h>
	#include <fftw3.h>
	#endif
#else
	#include <base/unique_handle.h>
	#include <Accelerate/Accelerate.h>
#endif

#include <player/fft.h>

namespace xamp::player {

MAKE_ENUM(WindowType, HANN, HAMMING)

static size_t ComplexSize(size_t size) noexcept {
	return (size / 2) + 1;
}

#ifndef USE_FFTW && defined(XAMP_OS_WIN)
class Window {
public:
	void Init(size_t size, WindowType type = WindowType::HAMMING) {
		window_ = MakeBuffer<float>(size);
		size_t m = size - 1;
		for (auto i = 0; i < size; ++i) {
			window_[i] = 0.54 - 0.46 * std::cos((2.0 * kPI * i) / m);
		}
	}

	void operator()(float const* samples, float* buffer, size_t size) {
		for (size_t i = 0; i < size; ++i) {
			buffer[i] = samples[i] * window_[i];
		}
	}
protected:
	Buffer<float> window_;
};
	
class FFT::FFTImpl {
public:
	void Init(size_t size) {
		complex_size_ = ComplexSize(size) * sizeof(float);
		input_ = MakeBuffer<float>(size);
		window_.Init(size);
	}

	std::valarray<Complex> Forward(float const* signals, size_t size) {
		window_(signals, input_.data(), size);
		
		std::valarray<Complex> output(Complex(), size);
		std::transform(&input_[0], &input_[size], &output[0], [](auto signal) {
			return Complex(signal, 0.0F);
			});
		Forward(output);
		return output;
	}

private:
	void Forward(std::valarray<Complex>& x) {
		const auto N = x.size();
		if (N <= 1) {
			return;
		}

		std::valarray<Complex> even = x[std::slice(0, N / 2, 2)];
		std::valarray<Complex> odd = x[std::slice(1, N / 2, 2)];

		Forward(even);
		Forward(odd);

		for (size_t k = 0; k < N / 2; ++k) {
			auto t = std::polar(1.0f, -2.0f * kPI * k / N) * odd[k];
			x[k] = even[k] + t;
			x[k + N / 2] = even[k] - t;
		}
	}

	void Inverse(std::valarray<Complex>& x) {
		x = x.apply(std::conj);
		Forward(x);
		x = x.apply(std::conj);
		x /= x.size();
	}

	size_t complex_size_{ 0 };
	Window window_;
	Buffer<float> input_;
};
#elif defined(XAMP_OS_MAC)
class Window {
public:
    void Init(size_t size, WindowType type = WindowType::HAMMING) {
        window_ = MakeBuffer<float>(size);
        (void)type;
        ::vDSP_hamm_window(window_.data(), size, vDSP_HANN_DENORM);
    }

    void operator()(float const* samples, float* buffer, size_t size) {
        ::vDSP_vmul(samples, 1, window_.data(), 1, buffer, 1, size);
    }
protected:
    Buffer<float> window_;
};

class FFT::FFTImpl {
public:
    FFTImpl() {
    }

    void Init(size_t size) {
        window_.Init(size);
        size_ = size;
        size_over2_ = size_ / 2;
        bin_.resize(size_over2_, Complex());
        log2n_size_ = std::log2(size);
        input_ = MakeBuffer<float>(size);
        output_ = MakeBuffer<float>(size);
        fft_setup_.reset(::vDSP_create_fftsetup(log2n_size_, FFT_RADIX2));
        re_ = MakeBuffer<float>(size_over2_);
        im_ = MakeBuffer<float>(size_over2_);
        split_complex_.realp = re_.data();
        split_complex_.imagp = im_.data();
    }

    std::valarray<Complex> Forward(float const* signals, size_t size) {
        window_(signals, input_.data(), size);

        ::vDSP_ctoz(reinterpret_cast<const COMPLEX*>(input_.data()), 2, &split_complex_, 1, size_over2_);
        ::vDSP_fft_zrip(fft_setup_.get(), &split_complex_, 1, log2n_size_, FFT_FORWARD);

        split_complex_.imagp[0] = 0.0;

        auto re = re_.data();
        auto im = im_.data();

        for (size_t i = 0; i < size_over2_; ++i) {
            bin_[i] = Complex(re[i], im[i]);
        }
        return bin_;
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
    FFTSetupHandle fft_setup_;
    Window window_;
    DSPSplitComplex split_complex_;
    Buffer<float> input_;
    Buffer<float> output_;
    Buffer<float> re_;
    Buffer<float> im_;
    std::valarray<Complex> bin_;
};
#else
class FFTWLib {
public:
	FFTWLib()
		: module_(LoadModule("libfftw3f-3.dll"))
		, fftwf_destroy_plan(module_, "fftwf_destroy_plan")
		, fftwf_malloc(module_, "fftwf_malloc")
		, fftwf_free(module_, "fftwf_free")
		, fftwf_plan_guru_split_dft_c2r(module_, "fftwf_plan_guru_split_dft_c2r")
		, fftwf_plan_guru_split_dft_r2c(module_, "fftwf_plan_guru_split_dft_r2c")
		, fftwf_execute_split_dft_r2c(module_, "fftwf_execute_split_dft_r2c")
		, fftwf_execute_split_dft_c2r(module_, "fftwf_execute_split_dft_c2r") {
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

#define FFTWDLL Singleton<FFTWLib>::GetInstance()

struct FFTWFloatPtrTraits final {
	static float* invalid() {
		return nullptr;
	}

	static void close(float* value) {
		FFTWDLL.fftwf_free(value);
	}
};

using FFTWPtr = UniqueHandle<float*, FFTWFloatPtrTraits>;

static FFTWPtr MakeBuffer(size_t size) {
	return FFTWPtr(static_cast<float*>(FFTWDLL.fftwf_malloc(sizeof(float) * size)));
}

class Window {
public:
	void Init(size_t size, WindowType type = WindowType::HAMMING) {
		window_ = MakeBuffer(size);
		const auto m = static_cast<float>(size - 1);
		for (size_t i = 0; i < size; ++i) {
			window_.get()[i] = 0.54f - 0.46f * std::cos((2.0f * kPI * static_cast<float>(i)) / m);
		}
	}

	void operator()(float const* samples, float* buffer, size_t size) {
		for (size_t i = 0; i < size; ++i) {
			buffer[i] = samples[i] * window_.get()[i];
		}
	}
protected:
	FFTWPtr window_;
};

class FFT::FFTImpl {
public:
	void Init(size_t size) {
		window_.Init(size);		
		complex_size_ = ComplexSize(size);
		output_.resize(complex_size_, Complex());
		input_ = MakeBuffer(size);
		re_ = MakeBuffer(complex_size_);
		im_ = MakeBuffer(complex_size_);

		fftw_iodim dim;
		dim.n = static_cast<int>(size);
		dim.is = 1;
		dim.os = 1;
		forward_.reset(FFTWDLL.fftwf_plan_guru_split_dft_r2c(1,
			&dim,
			0,
			nullptr,
			input_.get(),
			re_.get(),
			im_.get(),
			FFTW_ESTIMATE));
		if (!forward_.is_valid()) {
			throw LibrarySpecException("fftwf_plan_guru_split_dft_r2c return null.");
		}
		backward_.reset(FFTWDLL.fftwf_plan_guru_split_dft_c2r(1,
			&dim,
			0,
			nullptr,
			re_.get(),
			im_.get(),
			input_.get(),
			FFTW_ESTIMATE));
		if (!forward_.is_valid()) {
			throw LibrarySpecException("fftwf_plan_guru_split_dft_c2r return null.");
		}
	}

	std::valarray<Complex> Forward(float const* signals, size_t size) {
		window_(signals, input_.get(), size);

		FFTWDLL.fftwf_execute_split_dft_r2c(forward_.get(),
			input_.get(),
			re_.get(),
			im_.get());

		auto const* re = re_.get();
		auto const* im = im_.get();

		for (size_t i = 0; i < complex_size_; ++i) {
			output_[i] = Complex(re[i], im[i]);
		}
		return output_;
	}

private:
	struct FFTWPlanTraits final {
		static fftwf_plan invalid() {
			return nullptr;
		}

		static void close(fftwf_plan value) {
			FFTWDLL.fftwf_destroy_plan(value);
		}
	};

	using FFTWPlan = UniqueHandle<fftwf_plan, FFTWPlanTraits>;
	
	size_t complex_size_{ 0 };
	FFTWPlan forward_;
	FFTWPlan backward_;
	FFTWPtr input_;
	FFTWPtr re_;
	FFTWPtr im_;
	Window window_;
	std::valarray<Complex> output_;
};
#endif

FFT::FFT()
	: impl_(MakeAlign<FFTImpl>()) {
}

XAMP_PIMPL_IMPL(FFT)

void FFT::Init(size_t size) {
	impl_->Init(size);
}

std::valarray<Complex> FFT::Forward(float const* data, size_t size) {
	return impl_->Forward(data, size);
}
	
}
