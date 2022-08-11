#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/exception.h>
#include <stream/r8brainlib.h>

namespace xamp::stream {

R8brainLib::R8brainLib() try
#ifdef XAMP_OS_WIN
    : module_(LoadModule("r8bsrc.dll"))
#else
    : module_(LoadModule("libr8bsrc.dylib"))
#endif
	, XAMP_LOAD_DLL_API(r8b_create)
	, XAMP_LOAD_DLL_API(r8b_delete)
	, XAMP_LOAD_DLL_API(r8b_clear)
	, XAMP_LOAD_DLL_API(r8b_process) {
	PrefetchModule(module_);
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

}
