#include <stream/fftwlib.h>

namespace xamp::stream {

#ifdef XAMP_OS_WIN

FFTWLib::FFTWLib()
	: module_(LoadModule("libfftw3-3.dll"))
	, XAMP_LOAD_DLL_API(fftw_destroy_plan)
	, XAMP_LOAD_DLL_API(fftw_malloc)
	, XAMP_LOAD_DLL_API(fftw_free)
	, XAMP_LOAD_DLL_API(fftw_execute)
	, XAMP_LOAD_DLL_API(fftw_plan_dft_r2c_1d)
	, XAMP_LOAD_DLL_API(fftw_plan_dft_c2r_1d) {
}

FFTWFLib::FFTWFLib()
	: module_(LoadModule("libfftw3f-3.dll"))
	, XAMP_LOAD_DLL_API(fftwf_destroy_plan)
	, XAMP_LOAD_DLL_API(fftwf_malloc)
	, XAMP_LOAD_DLL_API(fftwf_free)
	, XAMP_LOAD_DLL_API(fftwf_plan_guru_split_dft_c2r)
	, XAMP_LOAD_DLL_API(fftwf_plan_guru_split_dft_r2c)
	, XAMP_LOAD_DLL_API(fftwf_execute_split_dft_r2c)
	, XAMP_LOAD_DLL_API(fftwf_execute_split_dft_c2r) {
}

#endif

}
