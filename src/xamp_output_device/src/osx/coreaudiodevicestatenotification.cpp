#include <output_device/osx/coreaudioexception.h>
#include <output_device/osx/coreaudiodevicestatenotification.h>

namespace xamp::output_device::osx {

inline constexpr AudioObjectPropertyAddress kAddOrRemoveDevicesPropertyAddress = {
    kAudioHardwarePropertyDevices,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
};

CoreAudioDeviceStateNotification::CoreAudioDeviceStateNotification(std::weak_ptr<IDeviceStateListener> callback)
    : callback_(callback) {
}

CoreAudioDeviceStateNotification::~CoreAudioDeviceStateNotification() {
    try {
        removePropertyListener();
    } catch (...) {
    }
}

void CoreAudioDeviceStateNotification::run() {
    addPropertyListener();
}

void CoreAudioDeviceStateNotification::removePropertyListener() {
    CoreAudioThrowIfError(::AudioObjectRemovePropertyListener(
        kAudioObjectSystemObject,
        &kAddOrRemoveDevicesPropertyAddress,
        &CoreAudioDeviceStateNotification::OnDefaultDeviceChangedCallback,
        this));
}

void CoreAudioDeviceStateNotification::addPropertyListener() {
    CoreAudioThrowIfError(::AudioObjectAddPropertyListener(
        kAudioObjectSystemObject,
        &kAddOrRemoveDevicesPropertyAddress,
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
        if (addresses[i].mSelector == kAddOrRemoveDevicesPropertyAddress.mSelector
            && addresses[i].mScope == kAddOrRemoveDevicesPropertyAddress.mScope
            && addresses[i].mElement == kAddOrRemoveDevicesPropertyAddress.mElement
            && context != nullptr) {
            if (auto callback = (*notification).callback_.lock()) {
                callback->onDeviceStateChange(DeviceState::DEVICE_STATE_ADDED, std::to_string(object));
            }
            break;
        }
    }
    return noErr;
}

}
