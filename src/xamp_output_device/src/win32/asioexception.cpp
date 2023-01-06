#if ENABLE_ASIO
#include <output_device/win32/asioexception.h>

namespace xamp::output_device::win32 {

std::string_view AsioException::ErrorMessage(ASIOError error) noexcept {
	switch (error) {
	case ASE_NotPresent:
		return "Hardware input or output is not present or available.";
	case ASE_HWMalfunction:
		return "Hardware is malfunctioning.";
	case ASE_InvalidParameter:
		return "Invalid input parameter.";
	case ASE_InvalidMode:
		return "Invalid mode.";
	case ASE_SPNotAdvancing:
		return "Sample position not advancing.";
	case ASE_NoClock:
		return "Sample clock or rate cannot be determined or is not present.";
	case ASE_NoMemory:
		return "Not enough memory to complete the request.";
	default:
		return "Unknown error";
	}
}

static Errors ToErrors(ASIOError error) {
	switch (error) {
	case ASE_NotPresent:
		return Errors::XAMP_ERROR_DEVICE_NOT_FOUND;
	default:
		return Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR;
	}
}

AsioException::AsioException(Errors error)
	: Exception(error) {
}

AsioException::AsioException(ASIOError error)
	: Exception(ToErrors(error)) {
	what_ = ErrorMessage(error);
}

}
#endif
