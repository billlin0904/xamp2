//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <stream/stream.h>
#include <base/singleton.h>
#include <base/dll.h>
#include <fftw3.h>

namespace xamp::stream {

class FFTWLib {
public:
	FFTWLib();

private:
	ModuleHandle module_;

public:
	XAMP_DECLARE_DLL(fftw_destroy_plan) fftw_destroy_plan;
	XAMP_DECLARE_DLL(fftw_malloc) fftw_malloc;
	XAMP_DECLARE_DLL(fftw_free) fftw_free;
	XAMP_DECLARE_DLL(fftw_execute) fftw_execute;
	XAMP_DECLARE_DLL(fftw_plan_dft_r2c_1d) fftw_plan_dft_r2c_1d;
	XAMP_DECLARE_DLL(fftw_plan_dft_c2r_1d) fftw_plan_dft_c2r_1d;
};

class FFTWFLib {
public:
	FFTWFLib();

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

#define FFTW_LIB Singleton<FFTWLib>::GetInstance()
#define FFTWF_LIB Singleton<FFTWFLib>::GetInstance()

template <typename T>
struct FFTWPtrTraits {
};

template <>
struct FFTWPtrTraits<double> final {
	void operator()(double* value) const {
		FFTW_LIB.fftw_free(value);
	}
};

template <>
struct FFTWPtrTraits<fftw_complex> final {
	void operator()(fftw_complex* value) const {
		FFTW_LIB.fftw_free(value);
	}
};

template <>
struct FFTWPtrTraits<float> final {
	void operator()(float* value) const {
		FFTWF_LIB.fftwf_free(value);
	}
};

template <>
struct FFTWPtrTraits<fftw_plan> final {
	void operator()(fftw_plan* value) const {
		FFTW_LIB.fftw_destroy_plan(*value);
	}
};

template <>
struct FFTWPtrTraits<fftwf_plan> final {
	void operator()(fftwf_plan value) const {
		FFTWF_LIB.fftwf_destroy_plan(value);
	}
};

struct FFTWFPlanTraits final {
	static fftwf_plan invalid() {
		return nullptr;
	}

	static void close(fftwf_plan value) {
		FFTWF_LIB.fftwf_destroy_plan(value);
	}
};

struct FFTWPlanTraits final {
	static fftw_plan invalid() {
		return nullptr;
	}

	static void close(fftw_plan value) {
		FFTW_LIB.fftw_destroy_plan(value);
	}
};

using FFTWFPlan = UniqueHandle<fftwf_plan, FFTWFPlanTraits>;
using FFTWPlan = UniqueHandle<fftw_plan, FFTWPlanTraits>;

using FFTWComplexArrayPtr = std::unique_ptr<fftw_complex[], FFTWPtrTraits<fftw_complex>>;
using FFTWDoubleArrayPtr = std::unique_ptr<double[], FFTWPtrTraits<double>>;

template <typename T>
std::unique_ptr<T[], FFTWPtrTraits<T>> MakeFFTWBuffer(size_t size) {
	return std::unique_ptr<
		T[],
		FFTWPtrTraits<T>
	>(static_cast<T*>(FFTW_LIB.fftw_malloc(sizeof(T) * size)));
}

}

#endif

