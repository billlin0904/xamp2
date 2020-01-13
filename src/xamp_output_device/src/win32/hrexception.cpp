#ifdef _WIN32

#include <iomanip>
#include <system_error>
#include <output_device/win32/hrexception.h>

namespace xamp::output_device::win32 {

void HRException::ThrowFromHResult(HRESULT hresult, std::string_view expr) {
	switch (hresult) {
	case AUDCLNT_E_DEVICE_IN_USE:
	case AUDCLNT_E_ENGINE_PERIODICITY_LOCKED:
	case AUDCLNT_E_ENGINE_FORMAT_LOCKED:
		throw DeviceInUseException();
	default:
		throw HRException(hresult, expr);
	}
}

HRException::HRException(HRESULT hresult, std::string_view expr)
	: PlatformSpecException(hresult)
	, hr_(hresult)
	, expr_(expr) {
	std::ostringstream ostr;
	ostr << "Hr code: 0x" << std::hex << hresult;
	message_ = ostr.str();
}

HRESULT HRException::GetHResult() const {
	return hr_;
}

const char* HRException::GetExpression() const {
	return expr_.data();
}

}
#endif
