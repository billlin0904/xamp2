#include <output_device/osx/coreaudioexception.h>
#include <output_device/osx/coreaudiodevicestatenotification.h>

namespace xamp::output_device::osx {

static constexpr AudioObjectPropertyAddress sAddOrRemoveDevicesPropertyAddress = {
    kAudioHardwarePropertyDevices,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
};

CoreAudioDeviceStateNotification::CoreAudioDeviceStateNotification(std::weak_ptr<DeviceStateListener> callback)
    : callback_(callback) {
}

CoreAudioDeviceStateNotification::~CoreAudioDeviceStateNotification() {
    try {
        RemovePropertyListener();
    } catch (...) {
    }
}

void CoreAudioDeviceStateNotification::Run() {
    AddPropertyListener();
}

void CoreAudioDeviceStateNotification::RemovePropertyListener() {
    CoreAudioThrowIfError(::AudioObjectRemovePropertyListener(
        kAudioObjectSystemObject,
        &sAddOrRemoveDevicesPropertyAddress,
        &CoreAudioDeviceStateNotification::OnDefaultDeviceChangedCallback,
        this));
}

void CoreAudioDeviceStateNotification::AddPropertyListener() {
    CoreAudioThrowIfError(::AudioObjectAddPropertyListener(
        kAudioObjectSystemObject,
        &sAddOrRemoveDevicesPropertyAddress,
        &CoreAudioDeviceStateNotification::OnDefaultDeviceChangedCallback,
        this));
}

OSStatus CoreAudioDeviceStateNotification::OnDefaultDeviceChangedCallback(
    AudioObjectID object,
    UInt32 num_addresses,
    AudioObjectPropertyAddress const addresses[],
    void* context) {
    auto notification = static_cast<CoreAudioDeviceStateNotification*>(context);
    for (UInt32 i = 0; i < num_addresses; ++i) {
        if (addresses[i].mSelector == sAddOrRemoveDevicesPropertyAddress.mSelector
            && addresses[i].mScope == sAddOrRemoveDevicesPropertyAddress.mScope
            && addresses[i].mElement == sAddOrRemoveDevicesPropertyAddress.mElement
            && context != nullptr) {
            if (auto callback = (*notification).callback_.lock()) {
                callback->OnDeviceStateChange(DeviceState::DEVICE_STATE_ADDED, std::to_wstring(object));
            }
            break;
        }
    }
    return noErr;
}

}
