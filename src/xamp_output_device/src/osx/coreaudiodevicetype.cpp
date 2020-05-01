#include <base/str_utilts.h>
#include <base/logger.h>

#include <output_device/osx/osx_str_utitl.h>
#include <output_device/osx/coreaudioexception.h>
#include <output_device/osx/coreaudiodevice.h>
#include <output_device/osx/coreaudiodevicetype.h>

namespace xamp::output_device::osx {

const ID CoreAudioDeviceType::Id("E6BB3BF2-F16A-489B-83EE-4A29755F42E4");

static std::vector<int32_t> GetAvailableSampleRates(AudioDeviceID id) {
    const AudioObjectPropertyAddress property = {
        kAudioDevicePropertyAvailableNominalSampleRates,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    std::vector<int32_t> samplerates;
    uint32_t dataSize = 0;
    auto result = ::AudioObjectGetPropertyDataSize(id, &property, 0, nullptr, &dataSize);
    if (result != kAudioHardwareNoError || dataSize == 0) {
        return samplerates;
    }

    UInt32 nRanges = dataSize / sizeof(AudioValueRange);
    std::vector<AudioValueRange> rangeList(nRanges);
    result = ::AudioObjectGetPropertyData(id, &property, 0, nullptr, &dataSize, rangeList.data());
    if (result != kAudioHardwareNoError) {
        return samplerates;
    }

    for (auto rangs : rangeList) {
        auto samplerate = static_cast<int32_t>(std::nearbyint(rangs.mMaximum));
        samplerates.push_back(samplerate);
    }
    return samplerates;
}

static bool IsSupportDopMode(AudioDeviceID id) {
    auto samplerates = GetAvailableSampleRates(id);
    return std::find_if(samplerates.begin(), samplerates.end(), [](auto samplerate) {
               // Minimal DOP DSD64 samplerate
               constexpr int32_t MIN_DOP_SAMPLERATE = 176400;
               return samplerate >= MIN_DOP_SAMPLERATE;
           }) != samplerates.end();
}

static std::wstring GetDeviceUid(AudioDeviceID id) {
    const AudioObjectPropertyAddress property = {
        kAudioDevicePropertyDeviceUID,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    CFStringRef uid = nullptr;
    UInt32 size = sizeof(uid);
    auto result = ::AudioObjectGetPropertyData(id,
                                               &property,
                                               0,
                                               nullptr,
                                               &size,
                                               &uid);
    if (result) {
        return L"";
    }
    return SysCFStringRefToWide(uid);
}

static std::wstring GetDeviceName(AudioDeviceID id, AudioObjectPropertySelector selector) {
    CFStringRef cfname;

    AudioObjectPropertyAddress property = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    uint32_t dataSize = sizeof(CFStringRef);
    property.mSelector = selector;
    auto result = ::AudioObjectGetPropertyData(id,
                                               &property,
                                               0,
                                               nullptr,
                                               &dataSize,
                                               &cfname);
    if (result != noErr) {
        return L"";
    }

    return ToStdWString(SysCFStringRefToUTF8(cfname));
}

static std::wstring GetPropertyName(AudioDeviceID id) {
    return GetDeviceName(id, kAudioObjectPropertyName);
}

static AudioDeviceID GetAudioDeviceIdByUid(bool is_input, const std::wstring& device_id) {
    AudioObjectPropertyAddress property_address = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    AudioDeviceID audio_device_id = kAudioObjectUnknown;
    UInt32 device_size = sizeof(audio_device_id);
    OSStatus result = -1;

    if (device_id.empty()) {
        property_address.mSelector = is_input ?
                                              kAudioHardwarePropertyDefaultInputDevice :
                                              kAudioHardwarePropertyDefaultOutputDevice;
        result = ::AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                              &property_address,
                                              0,
                                              nullptr,
                                              &device_size,
                                              &audio_device_id);
    } else {
        auto uid = SysWideToCFStringRef(device_id);

        AudioValueTranslation value;
        value.mInputData = &uid;
        value.mInputDataSize = sizeof(CFStringRef);
        value.mOutputData = &audio_device_id;
        value.mOutputDataSize = device_size;
        UInt32 translation_size = sizeof(AudioValueTranslation);

        property_address.mSelector = kAudioHardwarePropertyDeviceForUID;
        result = ::AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                              &property_address,
                                              0,
                                              nullptr,
                                              &translation_size,
                                              &value);
    }
    CoreAudioFailedLog(result);
    return audio_device_id;
}

static bool IsOutputDevice(AudioDeviceID id) {
    AudioObjectPropertyAddress property = {
        kAudioDevicePropertyStreams,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMaster
    };

    UInt32 dataSize = 0;
    auto result = ::AudioObjectGetPropertyDataSize(id,
                                                   &property,
                                                   0,
                                                   nullptr,
                                                   &dataSize);
    if (result != noErr) {
        return false;
    }
    return (dataSize / sizeof(AudioStreamID)) > 0;
}

CoreAudioDeviceType::CoreAudioDeviceType() {
}

void CoreAudioDeviceType::ScanNewDevice() {
    device_list_ = GetDeviceInfo();
}

std::string_view CoreAudioDeviceType::GetDescription() const {
    return "CoreAudio";
}

align_ptr<Device> CoreAudioDeviceType::MakeDevice(const std::wstring &device_id) {
    auto id = GetAudioDeviceIdByUid(false, device_id);
    return MakeAlign<Device, CoreAudioDevice>(id);
}

size_t CoreAudioDeviceType::GetDeviceCount() const {
    UInt32 data_size = 0;

    const AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    auto result = ::AudioObjectGetPropertyDataSize(
        kAudioObjectSystemObject,
        &propertyAddress,
        0,
        nullptr,
        &data_size);
    if (result != noErr) {
        return 0;
    }

    return data_size / sizeof(AudioDeviceID);
}

DeviceInfo CoreAudioDeviceType::GetDeviceInfo(int32_t device) const {
    return device_list_[device];
}

const ID & CoreAudioDeviceType::GetTypeId() const {
    return Id;
}

std::vector<DeviceInfo> CoreAudioDeviceType::GetDeviceInfo() const {
    std::vector<DeviceInfo> device_infos;

    AudioObjectPropertyAddress property = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    UInt32 data_size = sizeof(AudioDeviceID);
    auto device_count = GetDeviceCount();
    data_size *= device_count;

    std::vector<AudioDeviceID> device_list(device_count);
    property.mSelector = kAudioHardwarePropertyDevices;
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
        info.device_type_id = Id;
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

    const AudioObjectPropertyAddress property = {
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
    device_info.device_type_id = Id;
    device_info.is_default_device = true;
    return std::move(device_info);
}

}
