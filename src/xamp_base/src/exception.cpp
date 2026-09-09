#include <base/exception.h>

#include <base/base.h>
#include <base/platfrom_handle.h>
#include <base/dll.h>
#include <base/stacktrace.h>
#include <base/stl.h>

#include <sstream>
#include <algorithm>

XAMP_BASE_NAMESPACE_BEGIN

#define IMP_EXCEPTION_CLASS(ExceptionClassName, error) \
ExceptionClassName::ExceptionClassName(const std::string& message)\
	: Exception(error, message) {\
}\
ExceptionClassName::ExceptionClassName()\
    : Exception(error) {\
}

#ifdef XAMP_OS_WIN
#define GET_ERROR_MESSAGE() getPlatformErrorMessage(::GetLastError())
#else
#define GET_ERROR_MESSAGE() getPlatformErrorMessage(errno)
#endif

#ifdef XAMP_OS_WIN
std::string getPlatformErrorMessage(int32_t err) {
    return String::localeStringToUTF8(std::system_category().message(err));
}
std::string getLastErrorMessage() {
    return getPlatformErrorMessage(::GetLastError());
}
#else
std::string getPlatformErrorMessage(int32_t err) {
        return std::system_category().message(err);
}
std::string getLastErrorMessage() {
    return getPlatformErrorMessage(errno);
}
#endif

LibraryException::LibraryException(const std::string & message, std::string_view what)
    : Exception(Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR, message, what) {
}

Exception::Exception(std::string const& message, std::string_view what)
	: Exception(Errors::XAMP_ERROR_UNKNOWN, message, what) {
}

Exception::Exception(Errors error, const std::string& message, std::string_view what)
	: error_(error)
    , what_(what)
	, message_(message) {
    stacktrace_ = StackTrace{}.captureStack();
	if (what.empty()) {
        std::ostringstream ostr;
        ostr << error << "(" << errorToString(error) << ")";
        what_ = ostr.str();
	}
	if (message_.empty()) {
        std::ostringstream ostr;
        ostr << error << "(" << errorToString(error) << ")";
		message_ = ostr.str();
	}    
}

char const* Exception::getStackTrace() const {
    return stacktrace_.c_str();
}

char const * Exception::what() const noexcept {
    return what_.data();
}

Errors Exception::getError() const {
	return error_;
}

char const * Exception::getErrorMessage() const {
	return message_.c_str();
}

char const * Exception::getExpression() const {
	return "";
}

std::string_view Exception::errorToString(Errors error) {    
    return enumToString(error);
}

DeviceUnSupportedFormatException::DeviceUnSupportedFormatException(const AudioFormat &format)
	: Exception(Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT)
	, format_(format) {
	std::ostringstream ostr;
	ostr << "Device unsupported file format. (" << format_ << ")";
	message_ = ostr.str();
}

LoadDllFailureException::LoadDllFailureException(std::string_view dll_name)
	: Exception(Errors::XAMP_ERROR_LOAD_DLL_FAILURE)
	, dll_name_(dll_name) {
	std::ostringstream ostr;
	ostr << "load dll " << dll_name << " failure. (" << getLastErrorMessage() << ")";
	message_ = ostr.str();
}

NotFoundDllExportFuncException::NotFoundDllExportFuncException(std::string_view func_name)
    : Exception(Errors::XAMP_ERROR_NOT_FOUND_DLL_EXPORT_FUNC)
    , func_name_(func_name) {
    std::ostringstream ostr;
    ostr << "load dll function " << func_name << " failure. (" << getLastErrorMessage() << ")";
    message_ = ostr.str();
}

DeviceNotFoundException::DeviceNotFoundException()
    : Exception(Errors::XAMP_ERROR_DEVICE_NOT_FOUND) {
}

DeviceNotFoundException::DeviceNotFoundException(std::string_view device_name)
    : Exception(Errors::XAMP_ERROR_DEVICE_NOT_FOUND)
    , device_name_(device_name) {
    std::ostringstream ostr;
    ostr << "Device " << device_name << " not found.";
    message_ = ostr.str();
}

PlatformException::PlatformException()
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, GET_ERROR_MESSAGE()) {
}

PlatformException::PlatformException(std::string_view what)
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, GET_ERROR_MESSAGE(), what) {
}

PlatformException::PlatformException(int32_t err)
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, getPlatformErrorMessage(err)) {
}

PlatformException::PlatformException(std::string_view what, int32_t err)
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, getPlatformErrorMessage(err), what) {
}

IMP_EXCEPTION_CLASS(DeviceCreateFailureException, Errors::XAMP_ERROR_DEVICE_CREATE_FAILURE)
IMP_EXCEPTION_CLASS(DeviceInUseException, Errors::XAMP_ERROR_DEVICE_IN_USE)
IMP_EXCEPTION_CLASS(DeviceNeedSetMatchFormatException, Errors::XAMP_ERROR_DEVICE_NEED_SET_MATCH_FORMAT)
IMP_EXCEPTION_CLASS(FileNotFoundException, Errors::XAMP_ERROR_FILE_NOT_FOUND)
IMP_EXCEPTION_CLASS(NotSupportSampleRateException, Errors::XAMP_ERROR_NOT_SUPPORT_SAMPLE_RATE)
IMP_EXCEPTION_CLASS(NotSupportFormatException, Errors::XAMP_ERROR_NOT_SUPPORT_FORMAT)
IMP_EXCEPTION_CLASS(StopStreamTimeoutException, Errors::XAMP_ERROR_STOP_STREAM_TIMEOUT)
IMP_EXCEPTION_CLASS(SampleRateChangedException, Errors::XAMP_ERROR_SAMPLE_RATE_CHANGED)
IMP_EXCEPTION_CLASS(NotSupportResampleSampleRateException, Errors::XAMP_ERROR_NOT_SUPPORT_RESAMPLE_SAMPLE_RATE)
IMP_EXCEPTION_CLASS(NotSupportExclusiveModeException, Errors::XAMP_ERROR_NOT_SUPPORT_EXCLUSIVE_MODE)
IMP_EXCEPTION_CLASS(BufferOverflowException, Errors::XAMP_ERROR_NOT_BUFFER_OVERFLOW)

XAMP_BASE_NAMESPACE_END
