#include <base/uuid.h>
#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/asiodevice.h>
#include <output_device/asiodevicetype.h>
#include <output_device/audiodevicemanager.h>
#include <output_device/api.h>

namespace xamp::output_device {

AlignPtr<IAudioDeviceManager> MakeAudioDeviceManager() {
	return MakeAlign<IAudioDeviceManager, AudioDeviceManager>();
}

bool IsExclusiveDevice(DeviceInfo const& info) noexcept {
#ifdef XAMP_OS_WIN
    Uuid const device_type_id(info.device_type_id);
    return device_type_id == win32::ExclusiveWasapiDeviceType::Id
#if ENABLE_ASIO
        || device_type_id == ASIODeviceType::Id
#endif
        ;
#else
    (void)info;
    return false;
#endif
}

bool IsASIODevice(Uuid const& id) noexcept {
#if defined(ENABLE_ASIO) && defined(XAMP_OS_WIN)
    return id == ASIODeviceType::Id;
#else
    (void)id;
    return false;
#endif
}

void ResetASIODriver() {
#if defined(ENABLE_ASIO) && defined(XAMP_OS_WIN)
    AsioDevice::ResetDriver();
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


}
