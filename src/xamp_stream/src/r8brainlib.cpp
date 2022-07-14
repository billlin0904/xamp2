#include <base/logger.h>
#include <base/exception.h>
#include <stream/r8brainlib.h>

#ifdef XAMP_OS_WIN
namespace xamp::stream {

R8brainLib::R8brainLib() try
	: module_(LoadModule("r8bsrc.dll"))
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
#endif
