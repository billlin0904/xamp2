#include <base/exception.h>
#include <base/com_error_category.h>
#include <base/str_utilts.h>

#ifdef XAMP_OS_WIN
#include <Audioclient.h>
#include <comdef.h>
#include <Windows.h>

XAMP_BASE_NAMESPACE_BEGIN

std::string translatedHrError(HRESULT hr) {
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
        return "The audio stream was not stopped at the time of the start call.";
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
        return String::format("Unknown error: 0x{:08X}", hr);
    }
}


XAMP_BASE_NAMESPACE_END

#endif

