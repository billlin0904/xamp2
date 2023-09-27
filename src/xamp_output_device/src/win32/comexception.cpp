#include <output_device/win32/comexception.h>

#ifdef XAMP_OS_WIN
#include <output_device/win32/wasapi.h>

#include <base/base.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN
namespace {
    std::string MakeErrorMessage(HRESULT hr) {
        char result[16]{};
        switch (hr) {
        case AUDCLNT_E_SERVICE_NOT_RUNNING:
            return "AUDCLNT_E_SERVICE_NOT_RUNNING";
        case AUDCLNT_E_ALREADY_INITIALIZED:
            return "AUDCLNT_E_ALREADY_INITIALIZED";
        case AUDCLNT_E_WRONG_ENDPOINT_TYPE:
            return "AUDCLNT_E_WRONG_ENDPOINT_TYPE";
        case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED:
            return "AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED";
        case AUDCLNT_E_BUFFER_SIZE_ERROR:
            return "AUDCLNT_E_BUFFER_SIZE_ERROR";
        case AUDCLNT_E_CPUUSAGE_EXCEEDED:
            return "AUDCLNT_E_CPUUSAGE_EXCEEDED";
        case AUDCLNT_E_DEVICE_INVALIDATED:
            return "AUDCLNT_E_DEVICE_INVALIDATED";
        case AUDCLNT_E_DEVICE_IN_USE:
            return "AUDCLNT_E_DEVICE_IN_USE";
        case AUDCLNT_E_ENDPOINT_CREATE_FAILED:
            return "AUDCLNT_E_ENDPOINT_CREATE_FAILED";
        case AUDCLNT_E_INVALID_DEVICE_PERIOD:
            return "AUDCLNT_E_INVALID_DEVICE_PERIOD";
        case AUDCLNT_E_UNSUPPORTED_FORMAT:
            return "AUDCLNT_E_UNSUPPORTED_FORMAT";
        case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED:
            return "AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED";
        case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL:
            return "AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL";
        case AUDCLNT_E_NOT_INITIALIZED:
            return "AUDCLNT_E_NOT_INITIALIZED";
        case AUDCLNT_E_NOT_STOPPED:
            return "AUDCLNT_E_NOT_STOPPED";
        case AUDCLNT_E_EVENTHANDLE_NOT_SET:
            return "AUDCLNT_E_EVENTHANDLE_NOT_SET";
        case AUDCLNT_E_BUFFER_OPERATION_PENDING:
            return "AUDCLNT_E_BUFFER_OPERATION_PENDING";

        case E_POINTER:
            return "E_POINTER";
        case E_INVALIDARG:
            return "E_INVALIDARG";
        case E_OUTOFMEMORY:
            return "E_OUTOFMEMORY";
        case E_NOINTERFACE:
            return "E_NOINTERFACE";

        default:
            sprintf_s(result, _countof(result), "%08lX", hr);
            return result;
        }
    }

    std::string MakeFileNameAndLine(const Path& file_path, int32_t line_number) {
        std::ostringstream ostr;
        ostr << file_path.filename() << ":" << std::dec << line_number;
        return ostr.str();
    }
}

ComException::ComException(long hresult, std::string_view expr, const Path& file_path, int32_t line_number)
	: PlatformException(hresult)
	, hr_(hresult)
	, expr_(expr) {	
	message_ = MakeErrorMessage(hresult);
    file_name_and_line_ = MakeFileNameAndLine(file_path, line_number);
    what_ = message_;
}

long ComException::GetHResult() const {
	return hr_;
}

std::string ComException::GetFileNameAndLine() const {
    return file_name_and_line_;
}

const char* ComException::GetExpression() const noexcept {
	return expr_.data();
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif
