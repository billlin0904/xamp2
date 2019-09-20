#include <output_device/win32/hrexception.h>

namespace xamp::output_device::win32 {

HRException::HRException(HRESULT hresult)
	: hr_(hresult)
	, expr_("") {
}

HRException::HRException(HRESULT hresult, const char* expr)
	: hr_(hresult)
	, expr_(expr) {
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
