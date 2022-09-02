#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/exception.h>
#include <stream/fftwlib.h>

namespace xamp::stream {

#ifdef XAMP_OS_WIN

FFTWLib::FFTWLib() try
	: module_(LoadModule("libfftw3-3.dll"))
	, XAMP_LOAD_DLL_API(fftw_destroy_plan)
	, XAMP_LOAD_DLL_API(fftw_malloc)
	, XAMP_LOAD_DLL_API(fftw_free)
	, XAMP_LOAD_DLL_API(fftw_execute)
	, XAMP_LOAD_DLL_API(fftw_plan_dft_r2c_1d)
	, XAMP_LOAD_DLL_API(fftw_plan_dft_c2r_1d)
	, XAMP_LOAD_DLL_API(fftw_make_planner_thread_safe) {
	PrefetchModule(module_);
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

FFTWFLib::FFTWFLib() try
	: module_(LoadModule("libfftw3f-3.dll"))
	, XAMP_LOAD_DLL_API(fftwf_destroy_plan)
	, XAMP_LOAD_DLL_API(fftwf_malloc)
	, XAMP_LOAD_DLL_API(fftwf_free)
	, XAMP_LOAD_DLL_API(fftwf_plan_guru_split_dft_c2r)
	, XAMP_LOAD_DLL_API(fftwf_plan_guru_split_dft_r2c)
	, XAMP_LOAD_DLL_API(fftwf_execute_split_dft_r2c)
	, XAMP_LOAD_DLL_API(fftwf_execute_split_dft_c2r) {
	PrefetchModule(module_);
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

FFTWComplexArray MakeFFTWComplexArray(size_t size) {
	return FFTWComplexArray(static_cast<fftw_complex*>(FFTW_LIB.fftw_malloc(sizeof(fftw_complex) * size)));
}

FFTWDoubleArray MakeFFTWDoubleArray(size_t size) {
	return FFTWDoubleArray(static_cast<double*>(FFTW_LIB.fftw_malloc(sizeof(double) * size)));
}

FFTWPlan MakeFFTW(uint32_t fftsize, uint32_t times, FFTWDoubleArray& fftin, FFTWComplexArray& fftout) {
	return FFTWPlan(FFTW_LIB.fftw_plan_dft_r2c_1d(fftsize / times, fftin.get(), fftout.get(), FFTW_ESTIMATE));
}

FFTWPlan MakeIFFTW(uint32_t fftsize, uint32_t times, FFTWComplexArray& ifftin, FFTWDoubleArray& ifftout) {
	return FFTWPlan(FFTW_LIB.fftw_plan_dft_c2r_1d(fftsize / times, ifftin.get(), ifftout.get(), FFTW_ESTIMATE));
}

#endif

}
