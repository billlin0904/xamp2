#include <base/str_utilts.h>
#include <base/logger.h>

#include <output_device/osx/osx_str_utitl.h>
#include <output_device/osx/osx_utitl.h>
#include <output_device/osx/coreaudioexception.h>
#include <output_device/osx/coreaudiodevice.h>
#include <output_device/osx/coreaudiodevicetype.h>

namespace xamp::output_device::osx {

CoreAudioDeviceType::CoreAudioDeviceType() {
}

void CoreAudioDeviceType::ScanNewDevice() {
    device_list_ = GetDeviceInfo();
}

std::string_view CoreAudioDeviceType::GetDescription() const {
    return "CoreAudio";
}

AlignPtr<Device> CoreAudioDeviceType::MakeDevice(const std::string &device_id) {
    auto id = GetAudioDeviceIdByUid(false, device_id);
    return MakeAlign<Device, CoreAudioDevice>(id, false);
}

size_t CoreAudioDeviceType::GetDeviceCount() const {
    UInt32 data_size = 0;

    AudioObjectPropertyAddress const property = {
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

DeviceInfo CoreAudioDeviceType::GetDeviceInfo(uint32_t device) const {
    auto itr = device_list_.begin();
    if (device >= GetDeviceCount()) {
        throw DeviceNotFoundException();
    }
    std::advance(itr, device);
    return (*itr);
}

ID CoreAudioDeviceType::GetTypeId() const {
    return Id;
}

std::vector<DeviceInfo> CoreAudioDeviceType::GetDeviceInfo() const {
    std::vector<DeviceInfo> device_infos;

    AudioObjectPropertyAddress const property = {
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

    auto default_device_info = GetDefaultDeviceInfo();

    for (auto device_id : device_list) {
        if (!IsOutputDevice(device_id)) {
            continue;
        }

        DeviceInfo info;
        info.name = GetPropertyName(device_id);
        info.device_id = GetDeviceUid(device_id);
        info.device_type_id = GetTypeId();
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

std::optional<DeviceInfo> CoreAudioDeviceType::GetDefaultDeviceInfo() const {
    DeviceInfo device_info;

    AudioDeviceID id;
    UInt32 dataSize = sizeof(AudioDeviceID);

    AudioObjectPropertyAddress const property = {
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
    device_info.device_type_id = GetTypeId();
    device_info.is_default_device = true;
    return std::move(device_info);
}

}
