#include <output_device/osx/coreaudioexception.h>

namespace xamp::output_device::osx {

static const char * FormatErrorMessage(OSStatus status) {
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
        return "CoreAudio unknown error";
    }
}

CoreAudioException::CoreAudioException(OSStatus status) {
    message_ = FormatErrorMessage(status);
}

}
