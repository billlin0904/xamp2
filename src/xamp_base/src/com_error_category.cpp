#include <base/exception.h>
#include <base/com_error_category.h>

#ifdef XAMP_OS_WIN
#include <Audioclient.h>
#include <comdef.h>
#include <Windows.h>

using namespace xamp::base;

namespace {
    std::string WideToUtf8(const wchar_t* value) {
        if (value == nullptr || *value == L'\0') {
            return {};
        }

        const auto size = ::WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
        if (size <= 0) {
            return {};
        }

        std::string result(static_cast<size_t>(size - 1), '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), size, nullptr, nullptr);
        return result;
    }

    std::string NarrowMessage(const char* value) {
        return value != nullptr ? value : "";
    }

    std::string NarrowMessage(const wchar_t* value) {
        return WideToUtf8(value);
    }

    const char* GetAudioClientError(HRESULT hr) {
        switch (hr) {
        case AUDCLNT_E_NOT_INITIALIZED:
            return "The IAudioClient object is not initialized.";
        case AUDCLNT_E_ALREADY_INITIALIZED:
            return "The IAudioClient object is already initialized.";
        case AUDCLNT_E_WRONG_ENDPOINT_TYPE:
            return "The endpoint device is a capture device, not a rendering device.";
        case AUDCLNT_E_DEVICE_INVALIDATED:
            return "The audio device has been unplugged or otherwise made unavailable.";
        case AUDCLNT_E_NOT_STOPPED:
            return "The audio stream was not stopped at the time of the Start call.";
        case AUDCLNT_E_BUFFER_TOO_LARGE:
            return "The buffer is too large.";
        case AUDCLNT_E_OUT_OF_ORDER:
            return "A previous GetBuffer call is still in effect.";
        case AUDCLNT_E_UNSUPPORTED_FORMAT:
            return "The specified audio format is not supported.";
        case AUDCLNT_E_INVALID_SIZE:
            return "The wrong NumFramesWritten value.";
        case AUDCLNT_E_DEVICE_IN_USE:
            return "The endpoint device is already in use.";
        case AUDCLNT_E_BUFFER_OPERATION_PENDING:
            return "Buffer operation pending.";
        case AUDCLNT_E_THREAD_NOT_REGISTERED:
            return "The thread is not registered.";
        case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED:
            return "Exclusive mode is disabled on the device.";
        case AUDCLNT_E_ENDPOINT_CREATE_FAILED:
            return "Failed to create the audio endpoint.";
        case AUDCLNT_E_SERVICE_NOT_RUNNING:
            return "The Windows audio service is not running.";
        case AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED:
            return "The audio stream was not initialized for event-driven buffering.";
        case AUDCLNT_E_EXCLUSIVE_MODE_ONLY:
            return "Exclusive mode only.";
        case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL:
            return "The hnsBufferDuration and hnsPeriodicity parameters are not equal.";
        case AUDCLNT_E_EVENTHANDLE_NOT_SET:
            return "Event handle not set.";
        case AUDCLNT_E_INCORRECT_BUFFER_SIZE:
            return "Incorrect buffer size.";
        case AUDCLNT_E_BUFFER_SIZE_ERROR:
            return "Buffer size error.";
        case AUDCLNT_E_CPUUSAGE_EXCEEDED:
            return "CPU usage exceeded.";
        case AUDCLNT_E_BUFFER_ERROR:
            return "Buffer error.";
        case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED:
            return "Buffer size not aligned.";
        case AUDCLNT_E_INVALID_DEVICE_PERIOD:
            return "Invalid device period.";
        case AUDCLNT_E_INVALID_STREAM_FLAG:
            return "Invalid stream flag.";
        case AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE:
            return "The endpoint does not support offload mode.";
        case AUDCLNT_E_OUT_OF_OFFLOAD_RESOURCES:
            return "The endpoint does not have enough offload resources.";
        case AUDCLNT_E_OFFLOAD_MODE_ONLY:
            return "Offload mode only.";
        case AUDCLNT_E_NONOFFLOAD_MODE_ONLY:
            return "Non-offload mode only.";
        case AUDCLNT_E_RESOURCES_INVALIDATED:
            return "Audio resources were invalidated.";
        case AUDCLNT_E_RAW_MODE_UNSUPPORTED:
            return "Raw mode is not supported.";
        case AUDCLNT_E_ENGINE_PERIODICITY_LOCKED:
            return "Engine periodicity is locked.";
        case AUDCLNT_E_ENGINE_FORMAT_LOCKED:
            return "Engine format is locked.";
        default:
            return nullptr;
        }
    }
}

std::error_code make_error_code(com_error_enum e) {
    return { static_cast<int>(e), com_category() };
}

const std::error_category& com_category() {
    static com_error_category ecat;
    return ecat;
}

std::string com_error_category::message(int hr) const {
    if (const auto* message = GetAudioClientError(static_cast<HRESULT>(hr))) {
        return message;
    }
    return NarrowMessage(_com_error{ static_cast<HRESULT>(hr) }.ErrorMessage());
}

std::error_condition com_error_category::default_error_condition(int hr) const noexcept {
    if (HRESULT_CODE(hr) != 0 || hr == 0) {
        return std::system_category().default_error_condition(HRESULT_CODE(hr));
    }
    return { hr, com_category() };
}

std::system_error com_to_system_error(HRESULT hr, const char* msg) {
    return { { static_cast<com_error_enum>(hr) }, msg };
}

std::system_error com_to_system_error(HRESULT hr, const std::string& msg) {
    return com_to_system_error(hr, msg.c_str());
}

std::system_error com_to_system_error(HRESULT hr, const wchar_t* msg) {
    return com_to_system_error(hr, NarrowMessage(msg));
}

std::system_error com_to_system_error(HRESULT hr, const std::wstring& msg) {
    return com_to_system_error(hr, msg.c_str());
}

std::system_error com_to_system_error(const _com_error& e) {
    return com_to_system_error(e.Error(), e.ErrorMessage());
}

std::system_error com_to_system_error(HRESULT hr, IErrorInfo*) {
    return com_to_system_error(hr);
}

void WINAPI throw_translated_com_error(HRESULT hr, IErrorInfo* help) {
    throw com_to_system_error(hr, help);
}

#endif
