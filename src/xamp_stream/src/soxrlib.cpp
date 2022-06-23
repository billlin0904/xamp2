#include <base/logger.h>
#include <base/memory.h>
#include <stream/soxrlib.h>

namespace xamp::stream {

SoxrLib::SoxrLib() try
#ifdef XAMP_OS_WIN
    : module_(LoadModule("libsoxr.dll"))
#else
    : module_(LoadModule("libsoxr.dylib"))
#endif
    , XAMP_LOAD_DLL_API(soxr_quality_spec)
    , XAMP_LOAD_DLL_API(soxr_create)
    , XAMP_LOAD_DLL_API(soxr_process)
    , XAMP_LOAD_DLL_API(soxr_delete)
    , XAMP_LOAD_DLL_API(soxr_io_spec)
    , XAMP_LOAD_DLL_API(soxr_runtime_spec)
    , XAMP_LOAD_DLL_API(soxr_clear) {
    PrefetchModule(module_);
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

}
