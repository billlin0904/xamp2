#include <output_device/api.h>

#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/win32/asiodevice.h>
#include <output_device/win32/asiodevicetype.h>
#include <output_device/audiodevicemanager.h>

#include <base/uuid.h>

#ifdef XAMP_OS_MAC
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <output_device/osx/osx_utitl.h>
#include <output_device/osx/coreaudiodevicetype.h>
#include <output_device/osx/hogcoreaudiodevicetype.h>
#include <output_device/osx/coreaudiodevicestatenotification.h>
#else
#include <base/platfrom_handle.h>
#endif

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

#ifdef XAMP_OS_MAC
static struct IopmAssertion {
    IopmAssertion()
        : assertion_id(0) {
    }

    ~IopmAssertion() {
        Reset();
    }

    void PreventSleep() {
        if (assertion_id != 0) {
            Reset();
        }
        CFTimeInterval timeout = 5;
        ::IOPMAssertionCreateWithDescription(kIOPMAssertionTypePreventUserIdleSystemSleep,
                                             CFSTR("XAMP"),
                                             CFSTR("XAMP"),
                                             CFSTR("Prevents display sleep during playback"),
                                             CFSTR("/System/Library/CoreServices/powerd.bundle"),
                                             timeout,
                                             kIOPMAssertionTimeoutActionRelease,
                                             &assertion_id);
    }

    void Reset() {
        if (assertion_id == 0) {
            return;
        }
        ::IOPMAssertionRelease(assertion_id);
        assertion_id = 0;
    }

    IOPMAssertionID assertion_id;
} iopmAssertion;
#endif

AlignPtr<IAudioDeviceManager> MakeAudioDeviceManager() {
	return MakeAlign<IAudioDeviceManager, AudioDeviceManager>();
}

bool IsExclusiveDevice(const DeviceInfo& info) noexcept {
#ifdef XAMP_OS_WIN
    const Uuid device_type_id(info.device_type_id);
    return device_type_id == XAMP_UUID_OF(win32::ExclusiveWasapiDeviceType)
        || device_type_id == XAMP_UUID_OF(win32::AsioDeviceType)
        ;
#else
    (void)info;
    return false;
#endif
}

bool IsAsioDevice(const Uuid& id) noexcept {
#if defined(XAMP_OS_WIN)
    return id == XAMP_UUID_OF(win32::AsioDeviceType);
#else
    (void)id;
    return false;
#endif
}

void ResetAsioDriver() {
#if defined(XAMP_OS_WIN)
    win32::AsioDevice::ResetCurrentDriver();
#endif
}

void PreventSleep(bool allow) {
#ifdef XAMP_OS_WIN
    if (allow) {
        ::SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
    }
    else {
        ::SetThreadExecutionState(ES_CONTINUOUS);
    }
#else
    if (allow) {
        iopmAssertion.PreventSleep();
    }
    else {
        iopmAssertion.Reset();
    }
#endif
}

XAMP_OUTPUT_DEVICE_NAMESPACE_END
