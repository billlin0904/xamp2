#include <base/exception.h>

#include <base/base.h>
#include <base/platfrom_handle.h>
#include <base/stacktrace.h>
#include <base/stl.h>

#include <sstream>
#include <algorithm>

namespace xamp::base {

#define IMP_EXCEPTION_CLASS(ExceptionClassName, error) \
ExceptionClassName::ExceptionClassName(std::string const& message)\
	: Exception(error, message) {\
}\
ExceptionClassName::ExceptionClassName()\
    : Exception(error) {\
}

#ifdef XAMP_OS_WIN
#define GET_ERROR_MESSAGE() GetPlatformErrorMessage(::GetLastError())
#else
#define GET_ERROR_MESSAGE() GetPlatformErrorMessage(errno)
#endif

#ifdef XAMP_OS_WIN
std::string GetPlatformErrorMessage(int32_t err) {
    return String::LocaleStringToUTF8(std::system_category().message(err));
}
std::string GetLastErrorMessage() {
    return GetPlatformErrorMessage(::GetLastError());
}
#else
std::string GetPlatformErrorMessage(int32_t err) {
        return std::system_category().message(err);
}
std::string GetLastErrorMessage() {
    return GetPlatformErrorMessage(errno);
}
#endif

LibrarySpecException::LibrarySpecException(const std::string & message, std::string_view what)
    : Exception(Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR, message, what) {
}

Exception::Exception(std::string const& message, std::string_view what)
	: Exception(Errors::XAMP_ERROR_UNKNOWN, message, what) {
}

Exception::Exception(Errors error, const std::string& message, std::string_view what)
	: error_(error)
    , what_(what)
	, message_(message) {
    stacktrace_ = StackTrace{}.CaptureStack();
	if (what.empty()) {
        std::ostringstream ostr;
        ostr << error << "(" << ErrorToString(error) << ")";
        what_ = ostr.str();
	}
	if (message_.empty()) {
        std::ostringstream ostr;
        ostr << error << "(" << ErrorToString(error) << ")";
		message_ = ostr.str();
	}    
}

char const* Exception::GetStackTrace() const noexcept {
    return stacktrace_.c_str();
}

char const * Exception::what() const noexcept {
    return what_.data();
}

Errors Exception::GetError() const noexcept {
	return error_;
}

char const * Exception::GetErrorMessage() const noexcept {
	return message_.c_str();
}

char const * Exception::GetExpression() const noexcept {
	return "";
}

std::string_view Exception::ErrorToString(Errors error) {
    static const HashMap<Errors, const std::string_view> error_msgs {
        { Errors::XAMP_ERROR_SUCCESS, "Success." },
        { Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, "Platform spec error." },
        { Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR, "Library spec error." },
        { Errors::XAMP_ERROR_DEVICE_CREATE_FAILURE, "Failed to create the audio endpoint." },
        { Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT, "Device unsupported format." },
        { Errors::XAMP_ERROR_DEVICE_IN_USE, "Device in use." },
        { Errors::XAMP_ERROR_DEVICE_NOT_FOUND, "Device not found." },
        { Errors::XAMP_ERROR_FILE_NOT_FOUND, "File not found." },
        { Errors::XAMP_ERROR_NOT_SUPPORT_SAMPLERATE, "Not support samplerate." },
        { Errors::XAMP_ERROR_NOT_SUPPORT_FORMAT, "Not support format." },
        { Errors::XAMP_ERROR_LOAD_DLL_FAILURE, "Load dll failure." },
        { Errors::XAMP_ERROR_STOP_STREAM_TIMEOUT, "Stop stream thread timeout." },
        { Errors::XAMP_ERROR_NOT_SUPPORT_RESAMPLE_SAMPLERATE, "Resampler not support variable resample." },
        { Errors::XAMP_ERROR_SAMPLERATE_CHANGED, "SampleRate was changed." },
        { Errors::XAMP_ERROR_NOT_FOUND_DLL_EXPORT_FUNC, "Not found dll export function." },
        { Errors::XAMP_ERROR_NOT_SUPPORT_EXCLUSIVE_MODE, "Not support exclusive mode." },
        };
    auto const itr = error_msgs.find(error);
    if (itr != error_msgs.end()) {
        return (*itr).second;
    }
    return "";
}

DeviceUnSupportedFormatException::DeviceUnSupportedFormatException(AudioFormat const & format)
	: Exception(Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT)
	, format_(format) {
	std::ostringstream ostr;
	ostr << "Device unsupported file format." << format_;
	message_ = ostr.str();
}

LoadDllFailureException::LoadDllFailureException(std::string_view dll_name)
	: Exception(Errors::XAMP_ERROR_LOAD_DLL_FAILURE)
	, dll_name_(dll_name) {
	std::ostringstream ostr;
	ostr << "Load dll " << dll_name << " failure. " << GetLastErrorMessage();
	message_ = ostr.str();
}

NotFoundDllExportFuncException::NotFoundDllExportFuncException(std::string_view func_name)
    : Exception(Errors::XAMP_ERROR_NOT_FOUND_DLL_EXPORT_FUNC)
    , func_name_(func_name) {
    std::ostringstream ostr;
    ostr << "Load dll function " << func_name << " failure. " << GetLastErrorMessage();
    message_ = ostr.str();
}

PlatformSpecException::PlatformSpecException()
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, GET_ERROR_MESSAGE()) {
}

PlatformSpecException::PlatformSpecException(std::string_view what)
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, GET_ERROR_MESSAGE(), what) {
}

PlatformSpecException::PlatformSpecException(int32_t err)
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, GetPlatformErrorMessage(err)) {
}

PlatformSpecException::PlatformSpecException(std::string_view what, int32_t err)
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, GetPlatformErrorMessage(err), what) {
}

IMP_EXCEPTION_CLASS(DeviceCreateFailureException, Errors::XAMP_ERROR_DEVICE_CREATE_FAILURE)
IMP_EXCEPTION_CLASS(DeviceInUseException, Errors::XAMP_ERROR_DEVICE_IN_USE)
IMP_EXCEPTION_CLASS(DeviceNotFoundException, Errors::XAMP_ERROR_DEVICE_NOT_FOUND)
IMP_EXCEPTION_CLASS(FileNotFoundException, Errors::XAMP_ERROR_FILE_NOT_FOUND)
IMP_EXCEPTION_CLASS(NotSupportSampleRateException, Errors::XAMP_ERROR_NOT_SUPPORT_SAMPLERATE)
IMP_EXCEPTION_CLASS(NotSupportFormatException, Errors::XAMP_ERROR_NOT_SUPPORT_FORMAT)
IMP_EXCEPTION_CLASS(StopStreamTimeoutException, Errors::XAMP_ERROR_STOP_STREAM_TIMEOUT)
IMP_EXCEPTION_CLASS(SampleRateChangedException, Errors::XAMP_ERROR_SAMPLERATE_CHANGED);
IMP_EXCEPTION_CLASS(NotSupportResampleSampleRateException, Errors::XAMP_ERROR_NOT_SUPPORT_RESAMPLE_SAMPLERATE);
IMP_EXCEPTION_CLASS(NotSupportExclusiveModeException, Errors::XAMP_ERROR_NOT_SUPPORT_EXCLUSIVE_MODE)
IMP_EXCEPTION_CLASS(BufferOverflowException, Errors::XAMP_ERROR_NOT_BUFFER_OVERFLOW)

}
