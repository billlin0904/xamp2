#include <vector>
#include <algorithm>

#include <output_device/asioexception.h>

namespace xamp::output_device {

ASIOException::ASIOException(Errors error)
	: Exception(error) {
}

ASIOException::ASIOException(ASIOError error) {
	struct ASIOErrorMessages {
		ASIOError value;
		const char* message;
	};

	static const std::vector<ASIOErrorMessages> asio_error_messages {
		{ ASE_NotPresent,       "Hardware input or output is not present or available." },
		{ ASE_HWMalfunction,    "Hardware is malfunctioning." },
		{ ASE_InvalidParameter, "Invalid input parameter." },
		{ ASE_InvalidMode,      "Invalid mode." },
		{ ASE_SPNotAdvancing,   "Sample position not advancing." },
		{ ASE_NoClock,          "Sample clock or rate cannot be determined or is not present." },
		{ ASE_NoMemory,         "Not enough memory to complete the request." }
	};

	auto itr = std::find_if(asio_error_messages.begin(), asio_error_messages.end(),
		[error](auto error_msg) {
		return error_msg.value == error;
		});

	if (itr != asio_error_messages.end()) {
		message_ = (*itr).message;
	} else {
		message_ = "";
	}
}

}
