#include <base/str_utilts.h>
#include <base/logger.h>

#include <output_device/osx/coreaudiodevice.h>
#include <output_device/osx/coreaudiodevicetype.h>

namespace xamp::output_device::osx {

const ID CoreAudioDeviceType::Id = ID("E6BB3BF2-F16A-489B-83EE-4A29755F42E4");

static std::wstring CFSStringToStdWstring(CFStringRef &cfname) {
    std::string name;
    auto length = CFStringGetLength(cfname);
    std::vector<char> mname(length * 3 + 1);
    CFStringGetCString(cfname,
                       mname.data(),
                       length * 3 + 1,
                       kCFStringEncodingUTF8);
    name.append(mname.data(), strlen(mname.data()));
    CFRelease(cfname);
    return ToStdWString(name);
}

static std::vector<int32_t> GetAvailableSampleRates(AudioDeviceID id) {
    AudioObjectPropertyAddress property = {
        kAudioDevicePropertyAvailableNominalSampleRates,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    std::vector<int32_t> samplerates;
    uint32_t dataSize = 0;
    auto result = AudioObjectGetPropertyDataSize(id, &property, 0, nullptr, &dataSize);
    if (result != kAudioHardwareNoError || dataSize == 0) {
        return samplerates;
    }
    UInt32 nRanges = dataSize / sizeof(AudioValueRange);
    std::vector<AudioValueRange> rangeList(nRanges);
    result = AudioObjectGetPropertyData(id, &property, 0, nullptr, &dataSize, rangeList.data());
    if (result != kAudioHardwareNoError) {
        return samplerates;
    }
    for (auto rangs : rangeList) {
        auto samplerate = static_cast<int32_t>(std::nearbyint(rangs.mMaximum));
        XAMP_LOG_DEBUG("Device id: {} samplerate: {}", id, samplerate);
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
    AudioObjectPropertyAddress property = {
        kAudioDevicePropertyDeviceUID,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    CFStringRef uid = nullptr;
    UInt32 size = sizeof(uid);
    auto result = AudioObjectGetPropertyData(id,
                                        &property,
                                        0,
                                        nullptr,
                                        &size,
                                        &uid);
    if (result) {
        return L"";
    }
    return CFSStringToStdWstring(uid);
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
    auto result = AudioObjectGetPropertyData(id,
                                             &property,
                                             0,
                                             nullptr,
                                             &dataSize,
                                             &cfname);
    if (result != noErr) {
        return L"";
    }

    return CFSStringToStdWstring(cfname);
}

static std::wstring GetPropertyName(AudioDeviceID id) {
    return GetDeviceName(id, kAudioObjectPropertyName);
}

static bool IsOutputDevice(AudioDeviceID id) {
    AudioObjectPropertyAddress property = {
        kAudioDevicePropertyStreams,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMaster
    };

    UInt32 dataSize = 0;
    auto result = AudioObjectGetPropertyDataSize(id,
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

AlignPtr<Device> CoreAudioDeviceType::MakeDevice(const std::wstring &device_id) {
    std::wistringstream ostr(device_id);
    AudioDeviceID id = kAudioDeviceUnknown;
    ostr >> id;
    return MakeAlign<Device, CoreAudioDevice>(id);
}

int32_t CoreAudioDeviceType::GetDeviceCount() const {
    UInt32 data_size = 0;

    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    auto result = AudioObjectGetPropertyDataSize(
                kAudioObjectSystemObject,
                &propertyAddress,
                0,
                nullptr,
                &data_size);
    if (result != noErr) {
        return  0;
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
    auto result = AudioObjectGetPropertyData(
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
        info.device_id = std::to_wstring(device_id);
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

    AudioObjectPropertyAddress property = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    auto result = AudioObjectGetPropertyData(
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
    device_info.device_id = std::to_wstring(id);
    device_info.device_type_id = Id;
    device_info.is_default_device = true;
    return std::move(device_info);
}

}
