#include <stream/fftwlib.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/exception.h>

XAMP_STREAM_NAMESPACE_BEGIN

#if defined(XAMP_OS_WIN) || defined(XAMP_OS_MAC)

#if (USE_INTEL_MKL_LIB)

MKLLib::MKLLib() try
	: module_(OpenSharedLibrary("mkl_rt.2"))
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

#endif

FFTWFLib::FFTWFLib() try
    : module_(OpenSharedLibrary("fftw3f-3"))
	, XAMP_LOAD_DLL_API(fftwf_destroy_plan)
	, XAMP_LOAD_DLL_API(fftwf_malloc)
	, XAMP_LOAD_DLL_API(fftwf_free)
	, XAMP_LOAD_DLL_API(fftwf_plan_guru_split_dft_c2r)
	, XAMP_LOAD_DLL_API(fftwf_plan_guru_split_dft_r2c)
	, XAMP_LOAD_DLL_API(fftwf_execute_split_dft_r2c)
	, XAMP_LOAD_DLL_API(fftwf_execute_split_dft_c2r)
	, XAMP_LOAD_DLL_API(fftwf_plan_with_nthreads)
	, XAMP_LOAD_DLL_API(fftwf_init_threads)
	, XAMP_LOAD_DLL_API(fftwf_cleanup_threads) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

#endif

XAMP_STREAM_NAMESPACE_END
