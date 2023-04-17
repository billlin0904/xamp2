#include <stream/soxrlib.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/exception.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(LibSoxr);

SoxrLib::SoxrLib() try
    : logger_(LoggerManager::GetInstance().GetLogger(kLibSoxrLoggerName))
	, module_(OpenSharedLibrary("soxr"))
    , XAMP_LOAD_DLL_API(soxr_quality_spec)
    , XAMP_LOAD_DLL_API(soxr_create)
    , XAMP_LOAD_DLL_API(soxr_process)
    , XAMP_LOAD_DLL_API(soxr_delete)
    , XAMP_LOAD_DLL_API(soxr_io_spec)
    , XAMP_LOAD_DLL_API(soxr_runtime_spec)
    , XAMP_LOAD_DLL_API(soxr_clear) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

SoxrLib::~SoxrLib() = default;

XAMP_STREAM_NAMESPACE_END
