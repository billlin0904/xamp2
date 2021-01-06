#if ENABLE_ASIO
#include <vector>
#include <algorithm>

#include <output_device/asioexception.h>

namespace xamp::output_device {

std::string_view ASIOException::ErrorMessage(ASIOError error) noexcept {
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

ASIOException::ASIOException(Errors error)
	: Exception(error) {
}

ASIOException::ASIOException(ASIOError error) {
	what_ = ErrorMessage(error);
}

}
#endif
