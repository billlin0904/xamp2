#include <output_device/api.h>

#ifdef XAMP_OS_WIN
#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/win32/asiodevice.h>
#include <output_device/win32/asiodevicetype.h>
#endif
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
        reset();
    }

    void preventSleep() {
        if (assertion_id != 0) {
            reset();
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

    void reset() {
        if (assertion_id == 0) {
            return;
        }
        ::IOPMAssertionRelease(assertion_id);
        assertion_id = 0;
    }

    IOPMAssertionID assertion_id;
} iopmAssertion;
#endif

ScopedPtr<IAudioDeviceManager> makeAudioDeviceManager() {
	auto manager = makeAlign<IAudioDeviceManager, AudioDeviceManager>();
	manager->initial();
	return manager;
}

bool isExclusiveDevice(const DeviceInfo& info) {
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

bool isAsioDevice(const Uuid& id) {
#if defined(XAMP_OS_WIN)
    return id == XAMP_UUID_OF(win32::AsioDeviceType);
#else
    (void)id;
    return false;
#endif
}

void resetAsioDriver() {
#if defined(XAMP_OS_WIN)
    win32::AsioDevice::resetCurrentDriver();
#endif
}

void preventSleep(bool allow) {
#ifdef XAMP_OS_WIN
    if (allow) {
        ::SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
    }
    else {
        ::SetThreadExecutionState(ES_CONTINUOUS);
    }
#elif defined(XAMP_OS_MAC)
    if (allow) {
        iopmAssertion.preventSleep();
    }
    else {
        iopmAssertion.reset();
    }
#else
    (void)allow;
#endif
}

XAMP_OUTPUT_DEVICE_NAMESPACE_END
