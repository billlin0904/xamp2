#include <base/str_utilts.h>
#include <base/logger.h>

#include <output_device/osx/osx_str_utitl.h>
#include <output_device/osx/osx_utitl.h>
#include <output_device/osx/coreaudioexception.h>
#include <output_device/osx/coreaudiodevice.h>
#include <output_device/osx/coreaudiodevicetype.h>

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>

namespace xamp::output_device::osx {

class CoreAudioDeviceType::CoreAudioDeviceTypeImpl {
public:
    CoreAudioDeviceTypeImpl();

    ~CoreAudioDeviceTypeImpl() = default;

    void ScanNewDevice();

    AlignPtr<IOutputDevice> MakeDevice(const std::string &device_id);

    size_t GetDeviceCount() const;

    DeviceInfo GetDeviceInfo(uint32_t device) const;

    Vector<DeviceInfo> GetDeviceInfo() const;

    std::optional<DeviceInfo> GetDefaultDeviceInfo() const;

private:
    Vector<DeviceInfo> device_list_;
};

CoreAudioDeviceType::CoreAudioDeviceTypeImpl::CoreAudioDeviceTypeImpl() {
}

void CoreAudioDeviceType::CoreAudioDeviceTypeImpl::ScanNewDevice() {
    device_list_ = GetDeviceInfo();
}

AlignPtr<IOutputDevice> CoreAudioDeviceType::CoreAudioDeviceTypeImpl::MakeDevice(const std::string &device_id) {
    auto id = GetAudioDeviceIdByUid(false, device_id);
    return MakeAlign<IOutputDevice, CoreAudioDevice>(id, false);
}

size_t CoreAudioDeviceType::CoreAudioDeviceTypeImpl::GetDeviceCount() const {
    UInt32 data_size = 0;

    AudioObjectPropertyAddress constexpr property = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    auto result = ::AudioObjectGetPropertyDataSize(
        kAudioObjectSystemObject,
        &property,
        0,
        nullptr,
        &data_size);
    if (result != noErr) {
        return 0;
    }

    return data_size / sizeof(AudioDeviceID);
}

DeviceInfo CoreAudioDeviceType::CoreAudioDeviceTypeImpl::GetDeviceInfo(uint32_t device) const {
    auto itr = device_list_.begin();
    if (device >= GetDeviceCount()) {
        throw DeviceNotFoundException();
    }
    std::advance(itr, device);
    return (*itr);
}

Vector<DeviceInfo> CoreAudioDeviceType::CoreAudioDeviceTypeImpl::GetDeviceInfo() const {
    Vector<DeviceInfo> device_infos;

    AudioObjectPropertyAddress constexpr property = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    UInt32 data_size = sizeof(AudioDeviceID);
    auto device_count = GetDeviceCount();
    data_size *= device_count;

    std::vector<AudioDeviceID> device_list(device_count);
    auto result = ::AudioObjectGetPropertyData(
        kAudioObjectSystemObject,
        &property,
        0,
        nullptr,
        &data_size,
        device_list.data());
    if (result != noErr) {
        return device_infos;
    }

    device_infos.reserve(device_count);
    auto default_device_info = GetDefaultDeviceInfo();

    for (auto device_id : device_list) {
        if (!IsOutputDevice(device_id)) {
            continue;
        }

        DeviceInfo info;
        info.name = GetPropertyName(device_id);
        String::RTrim(info.name);
        info.device_id = GetDeviceUid(device_id);
        info.device_type_id = XAMP_UUID_OF(CoreAudioDeviceType);
        info.connect_type = GetDeviceConnectType(device_id);
        info.is_hardware_control_volume = SystemVolume(kAudioHardwareServiceDeviceProperty_VirtualMainVolume,
                                                       device_id).CanSetVolume();

        info.is_hardware_control_volume = false;
        // 用SampleRate判斷是否支援DOP有缺陷,
        // 由使用者判斷是否支援DOP.
        info.is_support_dsd = IsSupportDopMode(device_id);
        if (default_device_info) {
            if (default_device_info.value().device_id == info.device_id) {
                info.is_default_device = true;
            }
        }
        device_infos.push_back(info);
    }

    return device_infos;
}

std::optional<DeviceInfo> CoreAudioDeviceType::CoreAudioDeviceTypeImpl::GetDefaultDeviceInfo() const {
    DeviceInfo device_info;

    AudioDeviceID id;
    UInt32 dataSize = sizeof(AudioDeviceID);

    AudioObjectPropertyAddress constexpr property = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    auto result = ::AudioObjectGetPropertyData(
        kAudioObjectSystemObject,
        &property,
        0,
        nullptr,
        &dataSize,
        &id);
    if (result != noErr) {
        return std::nullopt;
    }
    device_info.name = GetPropertyName(id);
    device_info.device_id = GetDeviceUid(id);
    device_info.device_type_id = XAMP_UUID_OF(CoreAudioDeviceType);
    device_info.is_default_device = true;
    return std::move(device_info);
}

CoreAudioDeviceType::CoreAudioDeviceType()
    : impl_(MakeAlign<CoreAudioDeviceTypeImpl>()) {
}

XAMP_PIMPL_IMPL(CoreAudioDeviceType)

std::string_view CoreAudioDeviceType::GetDescription() const {
    return Description;
}

Uuid CoreAudioDeviceType::GetTypeId() const {
    return XAMP_UUID_OF(CoreAudioDeviceType);
}

void CoreAudioDeviceType::ScanNewDevice() {
    impl_->ScanNewDevice();
}

AlignPtr<IOutputDevice> CoreAudioDeviceType::MakeDevice(const std::string &device_id) {
    return impl_->MakeDevice(device_id);
}

size_t CoreAudioDeviceType::GetDeviceCount() const {
    return impl_->GetDeviceCount();
}

DeviceInfo CoreAudioDeviceType::GetDeviceInfo(uint32_t device) const {
    return impl_->GetDeviceInfo(device);
}

Vector<DeviceInfo> CoreAudioDeviceType::GetDeviceInfo() const {
    return impl_->GetDeviceInfo();
}

std::optional<DeviceInfo> CoreAudioDeviceType::GetDefaultDeviceInfo() const {
    return impl_->GetDefaultDeviceInfo();
}

}
