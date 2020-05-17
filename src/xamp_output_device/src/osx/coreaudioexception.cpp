#include <Security/Security.h>

#include <output_device/osx/osx_str_utitl.h>
#include <output_device/osx/coreaudioexception.h>

namespace xamp::output_device::osx {

static std::string FormatErrorMessage(OSStatus status) noexcept {
    switch (status) {
    case kAudioHardwareNotRunningError:
        return "kAudioHardwareNotRunningError";
    case kAudioHardwareUnspecifiedError:
        return "kAudioHardwareUnspecifiedError";
    case kAudioHardwareUnknownPropertyError:
        return "kAudioHardwareUnknownPropertyError";
    case kAudioHardwareBadPropertySizeError:
        return "kAudioHardwareBadPropertySizeError";
    case kAudioHardwareIllegalOperationError:
        return "kAudioHardwareIllegalOperationError";
    case kAudioHardwareBadObjectError:
        return "kAudioHardwareBadObjectError";
    case kAudioHardwareBadDeviceError:
        return "kAudioHardwareBadDeviceError";
    case kAudioHardwareBadStreamError:
        return "kAudioHardwareBadStreamError";
    case kAudioHardwareUnsupportedOperationError:
        return "kAudioHardwareUnsupportedOperationError";
    case kAudioDeviceUnsupportedFormatError:
        return "kAudioDeviceUnsupportedFormatError";
    case kAudioDevicePermissionsError:
        return "kAudioDevicePermissionsError";
    default:
        return "unknown error";
    }
}

CoreAudioException::CoreAudioException(OSStatus status) {
    message_ = ErrorToString(status) + " (" + FormatErrorMessage(status) + ").";
}

std::string CoreAudioException::ErrorToString(OSStatus status) {
    auto str = ::SecCopyErrorMessageString(status, nullptr);
    auto result = SysCFStringRefToUTF8(str);
    ::CFRelease(str);
    return result;
}

}
