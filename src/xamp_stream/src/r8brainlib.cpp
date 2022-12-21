#include <stream/r8brainlib.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/exception.h>

namespace xamp::stream {

R8brainLib::R8brainLib() try
    : module_(OpenSharedLibrary("r8bsrc"))
	, XAMP_LOAD_DLL_API(r8b_create)
	, XAMP_LOAD_DLL_API(r8b_delete)
	, XAMP_LOAD_DLL_API(r8b_clear)
	, XAMP_LOAD_DLL_API(r8b_process) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

}
