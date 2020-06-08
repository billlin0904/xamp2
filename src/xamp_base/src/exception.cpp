#include <sstream>
#include <algorithm>
#include <cmath>

#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#include <base/str_utilts.h>
#endif

#include <base/stl.h>
#include <base/exception.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN
std::string LocaleStringToUTF8(const std::string &str) {
    std::vector<wchar_t> wszTo(str.length() + 1);
    ::MultiByteToWideChar(CP_ACP,
        0,
        str.c_str(),
        -1, 
        wszTo.data(),
        static_cast<int>(str.length()));
    return ToUtf8String(wszTo.data());
}

std::string GetPlatformErrorMessage(int32_t err) {
    return LocaleStringToUTF8(std::system_category().message(err));
}
#else
std::string GetPlatformErrorMessage(int32_t err) {
        return std::system_category().message(err);
}
#endif

Exception::Exception(Errors error, const std::string& message, std::string_view what)
	: error_(error)
    , what_(what)
	, message_(message) {
	if (message_.empty()) {
        std::ostringstream ostr;
        ostr << error << "(" << ErrorToString(error) << ")";
		message_ = ostr.str();
	}
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
    static const RobinHoodHashMap<Errors, const std::string_view> error_msgs {
        { Errors::XAMP_ERROR_SUCCESS, "Success." },
        { Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, "Platform spec error." },
        { Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR, "Library spec error." },
        { Errors::XAMP_ERROR_DEVICE_NOT_INITIALIZED, "Device not initialized." },
        { Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT, "Device unsupported format." },
        { Errors::XAMP_ERROR_DEVICE_IN_USE, "Device in use." },
        { Errors::XAMP_ERROR_DEVICE_NOT_FOUND, "Device not found." },
        { Errors::XAMP_ERROR_FILE_NOT_FOUND, "File not found." },
        { Errors::XAMP_ERROR_NOT_SUPPORT_SAMPLERATE, "Not support samplerate." },
        { Errors::XAMP_ERROR_NOT_SUPPORT_FORMAT, "Not support format." },
        { Errors::XAMP_ERROR_LOAD_DLL_FAILURE, "Load dll failure." },
        { Errors::XAMP_ERROR_STOP_STREAM_TIMEOUT, "Stop stream thread timeout." },
        { Errors::XAMP_ERROR_NOT_SUPPORT_VARIABLE_RESAMPLE, "Resampler not support variable resample." },
        { Errors::XAMP_ERROR_SAMPLERATE_CHANGED, "Samplerate was changed." },
        { Errors::XAMP_ERROR_NOT_FOUND_DLL_EXPORT_FUNC, "Not found dll export function." },
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
	ostr << "Device unsupported file format " << format_ << ".";
	message_ = ostr.str();
}

LoadDllFailureException::LoadDllFailureException(std::string_view dll_name)
	: Exception(Errors::XAMP_ERROR_LOAD_DLL_FAILURE)
	, dll_name_(dll_name) {
	std::ostringstream ostr;
	ostr << "Load dll " << dll_name << " failure.";
	message_ = ostr.str();
}

PlatformSpecException::PlatformSpecException() 
#ifdef XAMP_OS_WIN
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, GetPlatformErrorMessage(::GetLastError())) {
#else
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, GetPlatformErrorMessage(errno)) {
#endif
}

PlatformSpecException::PlatformSpecException(int32_t err)
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, GetPlatformErrorMessage(err)) {
}

PlatformSpecException::PlatformSpecException(std::string_view what, int32_t err)
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, GetPlatformErrorMessage(err), what) {
}

NotSupportVariableSampleRateException::NotSupportVariableSampleRateException(int32_t input_samplerate, int32_t output_samplerate) 
	: Exception(Errors::XAMP_ERROR_NOT_SUPPORT_VARIABLE_RESAMPLE) {
	std::ostringstream ostr;
    const double max = (std::max)(input_samplerate, output_samplerate);
    const double min = (std::min)(input_samplerate, output_samplerate);
	ostr << "Resampler not support variable resample. " << input_samplerate << "Hz to " 
        << output_samplerate << "Hz ("
         << std::round(max / min * 100.0) / 100.0 << "x)";
	message_ = ostr.str();
}

#define IMP_EXCEPTION_CLASS(ExceptionClassName, error) \
ExceptionClassName::ExceptionClassName()\
    : Exception(error) {\
}\

IMP_EXCEPTION_CLASS(LibrarySpecErrorException, Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR)
IMP_EXCEPTION_CLASS(DeviceNotInititalzedException, Errors::XAMP_ERROR_DEVICE_NOT_INITIALIZED)
IMP_EXCEPTION_CLASS(DeviceInUseException, Errors::XAMP_ERROR_DEVICE_IN_USE)
IMP_EXCEPTION_CLASS(DeviceNotFoundException, Errors::XAMP_ERROR_DEVICE_NOT_FOUND)
IMP_EXCEPTION_CLASS(FileNotFoundException, Errors::XAMP_ERROR_FILE_NOT_FOUND)
IMP_EXCEPTION_CLASS(NotSupportSampleRateException, Errors::XAMP_ERROR_NOT_SUPPORT_SAMPLERATE)
IMP_EXCEPTION_CLASS(NotSupportFormatException, Errors::XAMP_ERROR_NOT_SUPPORT_FORMAT)
IMP_EXCEPTION_CLASS(StopStreamTimeoutException, Errors::XAMP_ERROR_STOP_STREAM_TIMEOUT)
IMP_EXCEPTION_CLASS(SampleRateChangedException, Errors::XAMP_ERROR_SAMPLERATE_CHANGED);
IMP_EXCEPTION_CLASS(NotFoundDllExportFuncException, Errors::XAMP_ERROR_NOT_FOUND_DLL_EXPORT_FUNC);

}
