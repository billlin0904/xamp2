#include <base/fftlib.h>

#include "fftlib_private.h"

#include <base/logger.h>
#include <base/exception.h>

#ifdef XAMP_OS_WIN

XAMP_BASE_NAMESPACE_BEGIN

MKLLib::MKLLib() try
	: mkl_core_(OpenSharedLibrary("mkl_core.2"))
	, libiomp_(OpenSharedLibrary("libiomp5md"))
	, mkl_intel_thread_(OpenSharedLibrary("mkl_intel_thread.2"))
	, mkl_cdft_core_(OpenSharedLibrary("mkl_cdft_core.2"))
	, mkl_def_(OpenSharedLibrary("mkl_def.2"))
	, mkl_mc3_(OpenSharedLibrary("mkl_mc3.2"))
	, mkl_avx2_(OpenSharedLibrary("mkl_avx2.2"))
	, mkl_avx512_(OpenSharedLibrary("mkl_avx512.2"))
	, module_(OpenSharedLibrary("mkl_rt.2"))
	, XAMP_LOAD_DLL_API(MKL_malloc)
	, XAMP_LOAD_DLL_API(MKL_free)
	, XAMP_LOAD_DLL_API(DftiErrorClass)
	, XAMP_LOAD_DLL_API(DftiFreeDescriptor)
	, XAMP_LOAD_DLL_API(DftiCreateDescriptor_s_1d)
	, XAMP_LOAD_DLL_API(DftiCreateDescriptor_s_md)
	, XAMP_LOAD_DLL_API(DftiCreateDescriptor_d_1d)
	, XAMP_LOAD_DLL_API(DftiCreateDescriptor_d_md)
	, XAMP_LOAD_DLL_API(DftiComputeForward)
	, DftiCreateDescriptor_(module_, "DftiCreateDescriptor")
	, XAMP_LOAD_DLL_API(DftiSetValue)
	, XAMP_LOAD_DLL_API(DftiCommitDescriptor)
	, XAMP_LOAD_DLL_API(DftiComputeBackward)
	, XAMP_LOAD_DLL_API(DftiErrorMessage) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

void LoadFFTLib() {
	SharedSingleton<MKLLib>::GetInstance();
}

XAMP_BASE_NAMESPACE_END

#endif
