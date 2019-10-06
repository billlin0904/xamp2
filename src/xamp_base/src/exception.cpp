#include <sstream>
#include <base/exception.h>

namespace xamp::base {

std::ostream& operator<<(std::ostream& ostr, Errors error) {
	switch (error) {
	case Errors::XAMP_ERROR_UNKNOWN:
		ostr << "Unknown error.";
		break;
	case Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR:
		ostr << "Platform spec error.";
		break;
	case Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR:
		ostr << "Library spec error.";
		break;
	case Errors::XAMP_ERROR_DEVICE_NOT_INITIALIZED:
		ostr << "Device not initialized.";
		break;
	case Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT:
		ostr << "Device unsupported format.";
		break;
	case Errors::XAMP_ERROR_DEVICE_IN_USE:
		ostr << "Device in use.";
		break;
	case Errors::XAMP_ERROR_DEVICE_NOT_FOUND:
		ostr << "Device not found.";
		break;
	case Errors::XAMP_ERROR_FILE_NOT_FOUND:
		ostr << "File not found.";
		break;
	case Errors::XAMP_ERROR_NOT_SUPPORT_SAMPLERATE:
		ostr << "Not support samplerate.";
		break;
	case Errors::XAMP_ERROR_NOT_SUPPORT_FORMAT:
		ostr << "Not support format.";
		break;
	case Errors::XAMP_ERROR_LOAD_DLL_FAILURE:
		ostr << "Load dll failure.";
		break;
	}
	return ostr;
}

Exception::Exception(Errors error, const std::string& message)
	: error_(error)
	, message_(message) {
	if (message_.empty()) {
		std::ostringstream ostr;
		ostr << error;
		message_ = ostr.str();
	}
}

char const* Exception::what() const {
	return what_.c_str();
}

Errors Exception::GetError() const {
	return error_;
}

const char * Exception::GetErrorMessage() const {
	return message_.c_str();
}

const char* Exception::GetExpression() const {
	return "";
}

}
