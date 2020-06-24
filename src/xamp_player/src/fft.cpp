// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <algorithm>
#include <cmath>
#include <cassert>

#include <base/base.h>
#include <base/math.h>
#include <base/enum.h>

#ifdef XAMP_OS_WIN
#define USE_FFTW
#else
#include <base/unique_handle.h>
#include <Accelerate/Accelerate.h>
#endif

#ifdef USE_FFTW
#include <base/dll.h>
#include <fftw3.h>
#endif

#include <player/fft.h>

namespace xamp::player {

MAKE_ENUM(WindowType,
          HANN,
		  HAMMING)

#ifndef USE_FFTW

#if 0
class FFT::FFTImpl {
public:
	void Init(size_t size) {
		complex_size_ = ComplexSize(size) * sizeof(float);
	}

	std::valarray<Complex> Forward(float const* signals, size_t size) {
        std::valarray<Complex> output(Complex(), size);
		std::transform(&signals[0], &signals[size], &output[0], [](auto signal) {
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

		std::valarray<std::complex<float>> even = x[std::slice(0, N / 2, 2)];
		std::valarray<std::complex<float>> odd = x[std::slice(1, N / 2, 2)];

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
};
#endif

class Window {
public:
	explicit Window(size_t size, WindowType type = WindowType::HAMMING) {
        window_ = MakeBuffer<float>(size);
        switch (type) {
        case WindowType::HANN:
            ::vDSP_hann_window(window_.get(), size, vDSP_HANN_DENORM);
            break;
        case WindowType::HAMMING:
            ::vDSP_hamm_window(window_.get(), size, vDSP_HANN_DENORM);
            break;
        }
    }

    void operator()(float const *samples, float *buffer, size_t size) {
        ::vDSP_vmul(samples, 1, window_.get(), 1, buffer, 1, size);
    }
protected:
    AlignBufferPtr<float> window_;
};

class FFT::FFTImpl {
public:
    explicit FFTImpl(size_t size)
        : window_(size) {
        Init(size);
    }

    void Init(size_t size) {
        size_ = size;
        size_over2_ = size_ / 2;
        log2n_size_ = Log2(size);
        input_ = MakeBuffer<float>(size);
        output_ = MakeBuffer<float>(size);
        fft_setup_.reset(::vDSP_create_fftsetup(log2n_size_, FFT_RADIX2));
        re_ = MakeBuffer<float>(size_over2_);
        im_ = MakeBuffer<float>(size_over2_);
        split_complex_.realp = re_.get();
        split_complex_.imagp = im_.get();
    }

    std::valarray<Complex> Forward(float const* signals, size_t size) {
        window_(signals, input_.get(), size);

        ::vDSP_ctoz(reinterpret_cast<const COMPLEX*>(input_.get()), 2, &split_complex_, 1, size_over2_);
        ::vDSP_fft_zrip(fft_setup_.get(), &split_complex_, 1, log2n_size_, FFT_FORWARD);

        split_complex_.imagp[0] = 0.0;

        auto re = re_.get();
        auto im = im_.get();

        std::valarray<Complex> output(Complex(), size_over2_);
        for (size_t i = 0; i < size_over2_; ++i) {
            output[i] = Complex(re[i], im[i]);
        }
        return output;
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

    size_t size_{0};
    size_t log2n_size_{0};
    size_t size_over2_{0};
    FFTSetupHandle fft_setup_;
    Window window_;
    DSPSplitComplex split_complex_;
    AlignBufferPtr<float> input_;
    AlignBufferPtr<float> output_;
    AlignBufferPtr<float> re_;
    AlignBufferPtr<float> im_;
};

#else

static size_t ComplexSize(size_t size) {
    return (size / 2) + 1;
}

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

	static FFTWLib& Instance() {
		static FFTWLib lib;
		return lib;
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

struct FFTWFloatPtrTraits final {
	template <typename T>
	void operator()(T value) const {
		FFTWLib::Instance().fftwf_free(value);
	}
};

using FFTWPtr = std::unique_ptr<float, FFTWFloatPtrTraits>;

static FFTWPtr MakeBuffer(size_t size) {
	return FFTWPtr(reinterpret_cast<float*>(FFTWLib::Instance().fftwf_malloc(sizeof(float) * size)));
}

class Window {
public:
	explicit Window(size_t size, WindowType type = WindowType::HAMMING) {
		window_ = MakeBuffer(size);
		switch (type) {
		case WindowType::HANN:
		case WindowType::HAMMING:
		{
			size_t m = size - 1;
			for (auto i = 0; i < size; ++i) {
				window_.get()[i] = 0.54 - 0.46 * std::cos((2.0 * kPI * i) / m);
			}
		}
		break;
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
    explicit FFTImpl(size_t size)
		: window_(size) {
        FFTWLib::Instance();
        Init(size);
	}

	void Init(size_t size) {
		complex_size_ = ComplexSize(size);
		data_ = MakeBuffer(size);
		re_ = MakeBuffer(complex_size_);
		im_ = MakeBuffer(complex_size_);

		fftw_iodim dim;
		dim.n = static_cast<int>(size);
		dim.is = 1;
		dim.os = 1;
		forward_.reset(FFTWLib::Instance().fftwf_plan_guru_split_dft_r2c(1,
			&dim,
			0,
			0,
			data_.get(),
			re_.get(),
			im_.get(),
			FFTW_ESTIMATE));

		backward_.reset(FFTWLib::Instance().fftwf_plan_guru_split_dft_c2r(1,
			&dim,
			0,
			0,
			re_.get(),
			im_.get(),
			data_.get(),
			FFTW_ESTIMATE));
	}

	std::valarray<Complex> Forward(float const* signals, size_t size) {		
		window_(signals, data_.get(), size);

		FFTWLib::Instance().fftwf_execute_split_dft_r2c(forward_.get(),
			data_.get(),
			re_.get(),
			im_.get());
		
		auto const re = re_.get();
		auto const im = im_.get();

		std::valarray<Complex> output(Complex(), complex_size_);
		for (size_t i = 0; i < complex_size_; ++i) {
			output[i] = Complex(re[i], im[i]);
		}

		return output;
	}

private:
	struct FFTWPlanTraits final {
		static fftwf_plan invalid() {
			return nullptr;
		}

		static void close(fftwf_plan value) {
			FFTWLib::Instance().fftwf_destroy_plan(value);
		}
	};	

	using FFTWPlan = UniqueHandle<fftwf_plan, FFTWPlanTraits>;
	

	size_t complex_size_{ 0 };
	FFTWPlan forward_;
	FFTWPlan backward_;
	FFTWPtr data_;
	FFTWPtr re_;
	FFTWPtr im_;	
	Window window_;
};

#endif

FFT::FFT(size_t size)
    : impl_(MakeAlign<FFTImpl>(size)) {
}

XAMP_PIMPL_IMPL(FFT)

void FFT::Init(size_t size) {
	impl_->Init(size);
}

std::valarray<Complex> FFT::Forward(float const* data, size_t size) {
	return impl_->Forward(data, size);
}

}
