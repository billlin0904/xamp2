#include <stream/r8brainlib.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/exception.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(R8brainLib);

R8brainLib::R8brainLib() try
    : logger_(LoggerManager::GetInstance().GetLogger(kR8brainLibLoggerName))
	, module_(OpenSharedLibrary("r8bsrc"))
	, XAMP_LOAD_DLL_API(r8b_create)
	, XAMP_LOAD_DLL_API(r8b_delete)
	, XAMP_LOAD_DLL_API(r8b_clear)
	, XAMP_LOAD_DLL_API(r8b_process) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

R8brainLib::~R8brainLib() = default;

XAMP_STREAM_NAMESPACE_END
