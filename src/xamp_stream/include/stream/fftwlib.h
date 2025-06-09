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

#define USE_INTEL_MKL_LIB 1

#if (USE_INTEL_MKL_LIB)
#define MKL_DIRECT_CALL
#include <fftw3.h>
#include <fftw/fftw3_mkl.h>
#include <mkl_service.h>
#include <mkl_dfti.h>
#include <mkl.h>
#else
#include <fftw3.h>
#endif

XAMP_STREAM_NAMESPACE_BEGIN

#if (USE_INTEL_MKL_LIB)

class MKLLib final {
public:
	MKLLib();

	XAMP_DISABLE_COPY(MKLLib)
private:
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL(MKL_malloc) MKL_malloc;
	XAMP_DECLARE_DLL(MKL_free) MKL_free;
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
	XAMP_DECLARE_DLL(DftiErrorMessage) DftiErrorMessage;
};

#define MKL_LIB SharedSingleton<MKLLib>::GetInstance()

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

#define FFTWF_LIB SharedSingleton<FFTWFLib>::GetInstance()

template <typename T>
struct FFTWPtrTraits;

template <typename T>
struct FFTWFPtrTraits;

template <>
struct FFTWFPtrTraits<float> final {
	void operator()(float* value) const {
		XAMP_EXPECTS(value != nullptr);
		FFTWF_LIB.fftwf_free(value);
	}
};

template <>
struct FFTWFPtrTraits<fftwf_plan> final {
	void operator()(fftwf_plan* value) const {
		XAMP_EXPECTS(value != nullptr);
		FFTWF_LIB.fftwf_destroy_plan(*value);
	}
};

struct FFTWFPlanTraits final {
	static fftwf_plan invalid() {
		return nullptr;
	}

	static void Close(fftwf_plan value) {
		XAMP_EXPECTS(value != nullptr);
		FFTWF_LIB.fftwf_destroy_plan(value);
	}
};

using FFTWFPlan = UniqueHandle<fftwf_plan, FFTWFPlanTraits>;
using FFTWFPtr = std::unique_ptr<float, FFTWFPtrTraits<float>>;

XAMP_STREAM_NAMESPACE_END

#endif

