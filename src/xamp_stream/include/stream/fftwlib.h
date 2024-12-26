//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#if defined(XAMP_OS_WIN) || defined(XAMP_OS_MAC)

#include <stream/stream.h>
#include <base/singleton.h>
#include <base/shared_singleton.h>
#include <base/dll.h>

#ifdef XAMP_OS_MAC
#define USE_INTEL_MKL_LIB 0
#endif

#if (USE_INTEL_MKL_LIB)
#define MKL_DIRECT_CALL
#include <fftw3.h>
#include <fftw3_mkl.h>
#include <mkl_service.h>
#else
#include <fftw3.h>
#endif

XAMP_STREAM_NAMESPACE_BEGIN

#if (USE_INTEL_MKL_LIB)

class FFTWLib final {
public:
	FFTWLib();

	XAMP_DISABLE_COPY(FFTWLib)

private:
	SharedLibraryHandle module_;

public:
	static void delete_plan(fftw_mkl_plan p);
	static fftw_mkl_plan new_plan(void);

	XAMP_DECLARE_DLL(DftiErrorClass) DftiErrorClass;
	XAMP_DECLARE_DLL(DftiFreeDescriptor) DftiFreeDescriptor;
	XAMP_DECLARE_DLL(DftiCreateDescriptor_s_1d) DftiCreateDescriptor_s_1d;
	XAMP_DECLARE_DLL(DftiCreateDescriptor_s_md) DftiCreateDescriptor_s_md;
	XAMP_DECLARE_DLL(DftiCreateDescriptor_d_1d) DftiCreateDescriptor_d_1d;
	XAMP_DECLARE_DLL(DftiCreateDescriptor_d_md) DftiCreateDescriptor_d_md;
	XAMP_DECLARE_DLL(DftiComputeForward) DftiComputeForward;
	XAMP_DECLARE_DLL(DftiCreateDescriptor) DftiCreateDescriptor_;
	XAMP_DECLARE_DLL(DftiSetValue) DftiSetValue;
	XAMP_DECLARE_DLL(DftiCommitDescriptor) DftiCommitDescriptor;
	XAMP_DECLARE_DLL(DftiComputeBackward) DftiComputeBackward;

	fftw_plan fftw_plan_guru64_dft_r2c(int rank,
		const fftw_iodim64* dims,
		int howmany_rank,
		const fftw_iodim64* howmany_dims,
		double* in,
		fftw_complex* out,
		unsigned flags);

	fftw_plan fftw_plan_guru64_dft_c2r(int rank,
		const fftw_iodim64* dims,
		int howmany_rank,
		const fftw_iodim64* howmany_dims,
		fftw_complex* in,
		double* out,
		unsigned flags);

	fftw_plan fftw_plan_dft_c2r(int rank, const int* n, fftw_complex* in, double* out, unsigned flags);
	fftw_plan fftw_plan_dft_r2c(int rank, const int* n, double* in, fftw_complex* out, unsigned flags);

	void fftw_destroy_plan(fftw_plan plan);
	void* fftw_malloc(size_t n);
	void fftw_free(void* p);
	void fftw_execute(const fftw_plan plan);
	fftw_plan fftw_plan_dft_r2c_1d(int n, double* in, fftw_complex* out, unsigned flags);
	fftw_plan fftw_plan_dft_c2r_1d(int n, fftw_complex* in, double* out, unsigned flags);
};
#else
class FFTWLib final {
public:
	FFTWLib();

	XAMP_DISABLE_COPY(FFTWLib)

private:
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL_NAME(fftw_destroy_plan);
	XAMP_DECLARE_DLL_NAME(fftw_malloc);
	XAMP_DECLARE_DLL_NAME(fftw_free);
	XAMP_DECLARE_DLL_NAME(fftw_execute);
	XAMP_DECLARE_DLL_NAME(fftw_plan_dft_r2c_1d);
	XAMP_DECLARE_DLL_NAME(fftw_plan_dft_c2r_1d);
};
#endif

class FFTWFLib final {
public:
	FFTWFLib();

	XAMP_DISABLE_COPY(FFTWFLib)

private:
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL_NAME(fftwf_destroy_plan);
	XAMP_DECLARE_DLL_NAME(fftwf_malloc);
	XAMP_DECLARE_DLL_NAME(fftwf_free);
	XAMP_DECLARE_DLL_NAME(fftwf_plan_guru_split_dft_c2r);
	XAMP_DECLARE_DLL_NAME(fftwf_plan_guru_split_dft_r2c);
	XAMP_DECLARE_DLL_NAME(fftwf_execute_split_dft_r2c);
	XAMP_DECLARE_DLL_NAME(fftwf_execute_split_dft_c2r);
	XAMP_DECLARE_DLL_NAME(fftwf_plan_with_nthreads);
	XAMP_DECLARE_DLL_NAME(fftwf_init_threads);
	XAMP_DECLARE_DLL_NAME(fftwf_cleanup_threads);
};

#define FFTW_LIB SharedSingleton<FFTWLib>::GetInstance()
#define FFTWF_LIB SharedSingleton<FFTWFLib>::GetInstance()

template <typename T>
struct FFTWPtrTraits;

template <>
struct FFTWPtrTraits<double> final {
	void operator()(double* value) const {
		XAMP_EXPECTS(value != nullptr);
		FFTW_LIB.fftw_free(value);
	}
};

template <>
struct FFTWPtrTraits<fftw_complex> final {
	void operator()(fftw_complex* value) const {
		XAMP_EXPECTS(value != nullptr);
		FFTW_LIB.fftw_free(value);
	}
};

template <>
struct FFTWPtrTraits<float> final {
	void operator()(float* value) const {
		XAMP_EXPECTS(value != nullptr);
		FFTWF_LIB.fftwf_free(value);
	}
};

template <>
struct FFTWPtrTraits<fftw_plan> final {
	void operator()(fftw_plan* value) const {
		XAMP_EXPECTS(value != nullptr);
		FFTW_LIB.fftw_destroy_plan(*value);
	}
};

template <>
struct FFTWPtrTraits<fftwf_plan> final {
	void operator()(fftwf_plan* value) const {
		XAMP_EXPECTS(value != nullptr);
		FFTWF_LIB.fftwf_destroy_plan(*value);
	}
};

struct FFTWFPlanTraits final {
	static fftwf_plan invalid() {
		return nullptr;
	}

	static void close(fftwf_plan value) {
		XAMP_EXPECTS(value != nullptr);
		FFTWF_LIB.fftwf_destroy_plan(value);
	}
};

struct FFTWPlanTraits final {
	static fftw_plan invalid() {
		return nullptr;
	}

	static void close(fftw_plan value) {
		XAMP_EXPECTS(value != nullptr);
		FFTW_LIB.fftw_destroy_plan(value);
	}
};

using FFTWFPlan = UniqueHandle<fftwf_plan, FFTWFPlanTraits>;
using FFTWPlan = UniqueHandle<fftw_plan, FFTWPlanTraits>;

using FFTWComplexArray = std::unique_ptr<fftw_complex[], FFTWPtrTraits<fftw_complex>>;
using FFTWDoubleArray = std::unique_ptr<double[], FFTWPtrTraits<double>>;

template <typename T>
std::unique_ptr<T[], FFTWPtrTraits<T>> MakeFFTWBuffer(size_t size) {
	return std::unique_ptr<
		T[],
		FFTWPtrTraits<T>
	>(static_cast<T*>(FFTW_LIB.fftw_malloc(sizeof(T) * size)));
}

FFTWComplexArray MakeFFTWComplexArray(size_t size);

FFTWDoubleArray MakeFFTWDoubleArray(size_t size);

FFTWPlan MakeFFTW(uint32_t fft_size, uint32_t times, FFTWDoubleArray& fft_in, FFTWComplexArray& fft_out);

FFTWPlan MakeIFFTW(uint32_t fft_size, uint32_t times, FFTWComplexArray& ifft_in, FFTWDoubleArray& ifft_out);

XAMP_STREAM_NAMESPACE_END

#endif

