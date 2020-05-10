#include <cmath>

#include <algorithm>

#include <base/str_utilts.h>

#include <output_device/osx/coreaudioexception.h>
#include <output_device/osx/osx_utitl.h>
#include <output_device/osx/osx_str_utitl.h>

namespace xamp::output_device::osx {

// Minimal DOP DSD64 samplerate
constexpr int32_t kMinDopSamplerate = 176400;

std::vector<uint32_t> GetAvailableSampleRates(AudioDeviceID id) {
    const AudioObjectPropertyAddress property = {
        kAudioDevicePropertyAvailableNominalSampleRates,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    std::vector<uint32_t> samplerates;
    uint32_t dataSize = 0;
    auto result = ::AudioObjectGetPropertyDataSize(id, &property, 0, nullptr, &dataSize);
    if (result != kAudioHardwareNoError || dataSize == 0) {
        CoreAudioFailedLog(result);
        return samplerates;
    }

    UInt32 nRanges = dataSize / sizeof(AudioValueRange);
    std::vector<AudioValueRange> rangeList(nRanges);
    result = ::AudioObjectGetPropertyData(id, &property, 0, nullptr, &dataSize, rangeList.data());
    if (result != kAudioHardwareNoError) {
        CoreAudioFailedLog(result);
        return samplerates;
    }

    for (auto rangs : rangeList) {
        auto samplerate = static_cast<uint32_t>(std::nearbyint(rangs.mMaximum));
        samplerates.push_back(samplerate);
    }
    return samplerates;
}

bool IsSupportSampleRate(AudioDeviceID id, uint32_t samplerate) {
    auto device_samplerates = GetAvailableSampleRates(id);
    return std::find(device_samplerates.begin(),
                     device_samplerates.end(),
                     samplerate) != device_samplerates.end();
}

bool IsSupportDopMode(AudioDeviceID id) {
    auto samplerates = GetAvailableSampleRates(id);
    return std::find_if(samplerates.begin(), samplerates.end(), [](auto samplerate) {
               return samplerate >= kMinDopSamplerate;
           }) != samplerates.end();
}

std::wstring GetDeviceUid(AudioDeviceID id) {
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
        CoreAudioFailedLog(result);
        return L"";
    }
    return SysCFStringRefToWide(uid);
}

std::wstring GetDeviceName(AudioDeviceID id, AudioObjectPropertySelector selector) {
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
        CoreAudioFailedLog(result);
        return L"";
    }

    return ToStdWString(SysCFStringRefToUTF8(cfname));
}

std::wstring GetPropertyName(AudioDeviceID id) {
    return GetDeviceName(id, kAudioObjectPropertyName);
}

AudioDeviceID GetAudioDeviceIdByUid(bool is_input, const std::wstring& device_id) {
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

bool IsOutputDevice(AudioDeviceID id) {
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
        CoreAudioFailedLog(result);
        return false;
    }
    return (dataSize / sizeof(AudioStreamID)) > 0;
}

}
