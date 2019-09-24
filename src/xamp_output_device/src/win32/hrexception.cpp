#include <system_error>
#include <output_device/win32/hrexception.h>

namespace xamp::output_device::win32 {

HRException::HRException(HRESULT hresult, const char* expr)
	: Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, std::system_category().message(hresult))
	, hr_(hresult)
	, expr_("") {
}

HRException::HRException(HRESULT hresult, Errors error)
	: hr_(hresult)
	, expr_("") {
}

HRESULT HRException::GetHResult() const {
	return hr_;
}

const char* HRException::GetExpression() const {
	return expr_;
}

}
