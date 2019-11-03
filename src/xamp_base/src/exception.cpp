#include <sstream>
#include <base/exception.h>

namespace xamp::base {

std::ostream& operator<<(std::ostream& ostr, Errors error) {
	switch (error) {
	case Errors::XAMP_ERROR_SUCCESS:
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
    case Errors::XAMP_ERROR_STOP_STREAM_TIMEOUT:
        ostr << "Stop stream timeout.";
        break;
    case Errors::_MAX_XAMP_ERROR_:
        break;
	}
	return ostr;
}

Exception::Exception(Errors error, const std::string& message, const char* what)
	: error_(error)
    , what_(what)
	, message_(message) {
	if (message_.empty()) {
		std::ostringstream ostr;
		ostr << error;
		message_ = ostr.str();
	}
}

char const* Exception::what() const noexcept {
    return what_;
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

#define IMP_EXCEPTION_CLASS(ExceptionClassName, error) \
ExceptionClassName::ExceptionClassName()\
    : Exception(error) {\
}\

IMP_EXCEPTION_CLASS(LibrarySpecErrorException, Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR)
IMP_EXCEPTION_CLASS(DeviceNotInititalzedException, Errors::XAMP_ERROR_DEVICE_NOT_INITIALIZED)
IMP_EXCEPTION_CLASS(DeviceInUseException, Errors::XAMP_ERROR_DEVICE_IN_USE)
IMP_EXCEPTION_CLASS(DeviceUnSupportedFormatException, Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT)
IMP_EXCEPTION_CLASS(DeviceNotFoundException, Errors::XAMP_ERROR_DEVICE_NOT_FOUND)
IMP_EXCEPTION_CLASS(FileNotFoundException, Errors::XAMP_ERROR_FILE_NOT_FOUND)
IMP_EXCEPTION_CLASS(NotSupportSampleRateException, Errors::XAMP_ERROR_NOT_SUPPORT_SAMPLERATE)
IMP_EXCEPTION_CLASS(NotSupportFormatException, Errors::XAMP_ERROR_NOT_SUPPORT_FORMAT)
IMP_EXCEPTION_CLASS(LoadDllFailureException, Errors::XAMP_ERROR_LOAD_DLL_FAILURE)
IMP_EXCEPTION_CLASS(StopStreamTimeoutException, Errors::XAMP_ERROR_STOP_STREAM_TIMEOUT)

}
