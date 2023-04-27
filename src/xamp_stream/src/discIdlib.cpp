#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/dll.h>
#include <stream/discIdlib.h>

#ifdef XAMP_OS_WIN

XAMP_STREAM_NAMESPACE_BEGIN

DiscIdLib::DiscIdLib() try
	: module_(OpenSharedLibrary("discid"))
	, XAMP_LOAD_DLL_API(discid_new)
	, XAMP_LOAD_DLL_API(discid_free)
	, XAMP_LOAD_DLL_API(discid_read)
	, XAMP_LOAD_DLL_API(discid_get_id)
	, XAMP_LOAD_DLL_API(discid_get_freedb_id)
	, XAMP_LOAD_DLL_API(discid_get_submission_url)
	, XAMP_LOAD_DLL_API(discid_get_default_device)
	, XAMP_LOAD_DLL_API(discid_get_error_msg) {
}
catch (const base::Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}

XAMP_STREAM_NAMESPACE_END

#endif