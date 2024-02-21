#include <base/base.h>

#ifdef XAMP_OS_MAC

#include <cmath>
#include <algorithm>

#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/str_utilts.h>

#include <output_device/osx/coreaudioexception.h>
#include <output_device/osx/osx_utitl.h>
#include <output_device/osx/osx_str_utitl.h>

namespace xamp::output_device::osx {

// Minimal DOP DSD64 samplerate
inline constexpr int32_t kMinDopSamplerate = 176400;

SystemVolume::SystemVolume(AudioObjectPropertySelector selector, AudioDeviceID device_id) noexcept
    : device_id_ (device_id) {
    if (device_id != kAudioObjectUnknown) {
        property_.mElement  = kAudioObjectPropertyElementMaster;
        property_.mSelector = selector;
        property_.mScope    = kAudioDevicePropertyScopeOutput;
        return;
    }
    property_.mScope    = kAudioObjectPropertyScopeGlobal;
    property_.mElement  = kAudioObjectPropertyElementMaster;
    property_.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    if (::AudioObjectHasProperty(kAudioObjectSystemObject, &property_)) {
        UInt32 deviceIDSize = sizeof (device_id_);
        OSStatus status = ::AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                       &property_,
                                                       0,
                                                       nullptr,
                                                       &deviceIDSize,
                                                       &device_id_);
        if (status == noErr) {
            property_.mElement  = kAudioObjectPropertyElementMaster;
            property_.mSelector = selector;
            property_.mScope    = kAudioDevicePropertyScopeOutput;
            if (!::AudioObjectHasProperty(device_id_, &property_)) {
                device_id_ = kAudioObjectUnknown;
            }
        }
    }
}

double SystemVolume::GetGain() const {
    Float32 gain = 0;
    if (device_id_ != kAudioObjectUnknown) {
        UInt32 size = sizeof(gain);
        CoreAudioThrowIfError(::AudioObjectGetPropertyData(device_id_,
                                                           &property_,
                                                           0,
                                                           nullptr,
                                                           &size,
                                                           &gain));
    }
    return static_cast<double>(gain);
}

void SystemVolume::SetGain(float gain) const {
    if (device_id_ != kAudioObjectUnknown && CanSetVolume()) {
        Float32 newVolume = gain;
        UInt32 size = sizeof(newVolume);
        CoreAudioThrowIfError(::AudioObjectSetPropertyData(device_id_,
                                                           &property_,
                                                           0,
                                                           nullptr,
                                                           size,
                                                           &newVolume));
    }
}

float SystemVolume::GetBlance(AudioObjectPropertyScope scope) const {
    AudioObjectPropertyAddress virtualMasterBalanceAddress {
        kAudioHardwareServiceDeviceProperty_VirtualMainVolume,
        scope,
        kAudioObjectPropertyElementMaster
    };

    UInt32 virtualMasterVolumePropertySize = sizeof(Float32);
    Float32 outVirtualMasterBalance = 0;
    CoreAudioThrowIfError(::AudioObjectGetPropertyData(device_id_,
                                                       &virtualMasterBalanceAddress,
                                                       0,
                                                       nullptr,
                                                       &virtualMasterVolumePropertySize,
                                                       &outVirtualMasterBalance));
    return outVirtualMasterBalance;
}

void SystemVolume::SetBlance(float blance, AudioObjectPropertyScope scope) {
    AudioObjectPropertyAddress virtualMasterBalanceAddress {
        kAudioHardwareServiceDeviceProperty_VirtualMainBalance,
        scope,
        kAudioObjectPropertyElementMaster
    };

    UInt32 size = sizeof(blance);
    CoreAudioThrowIfError(::AudioObjectSetPropertyData(device_id_,
                                                       &virtualMasterBalanceAddress,
                                                       0,
                                                       nullptr,
                                                       size,
                                                       &blance));
}

bool SystemVolume::IsMuted() const {
    UInt32 muted = 0;
    if (device_id_ != kAudioObjectUnknown) {
        UInt32 size = sizeof(muted);
        CoreAudioThrowIfError(::AudioObjectGetPropertyData(device_id_,
                                                           &property_,
                                                           0,
                                                           nullptr,
                                                           &size,
                                                           &muted));
    }
    return muted != 0;
}

void SystemVolume::SetMuted(bool mute) const {
    if (device_id_ != kAudioObjectUnknown && CanSetVolume()) {
        UInt32 newMute = mute ? 1 : 0;
        UInt32 size = sizeof(newMute);
        CoreAudioThrowIfError(::AudioObjectSetPropertyData(device_id_,
                                                           &property_,
                                                           0,
                                                           nullptr,
                                                           size,
                                                           &newMute));
    }
}

bool SystemVolume::HasProperty() const noexcept {
    return HasProperty(property_);
}

bool SystemVolume::HasProperty(const AudioObjectPropertyAddress &property) const noexcept {
    return ::AudioObjectHasProperty(device_id_, &property) > 0;
}

bool SystemVolume::CanSetVolume() const noexcept {
    Boolean is_settable = false;
    return ::AudioObjectIsPropertySettable(device_id_,
                                           &property_,
                                           &is_settable) == noErr && is_settable;
}

std::vector<std::string> GetSystemUsbPath() {
    std::vector<std::string> usb_device;
    auto matching_dict = ::IOServiceMatching(kIOUSBDeviceClassName);
    if (matching_dict == nullptr) {
        return usb_device;
    }

    io_iterator_t iter;
    auto kr = ::IOServiceGetMatchingServices(kIOMasterPortDefault, matching_dict, &iter);
    if (kr != KERN_SUCCESS) {
        return usb_device;
    }

    io_service_t device;
    while ((device = ::IOIteratorNext(iter))) {
        SInt32 score;
        IOCFPlugInInterface** plugin = nullptr;
        kern_return_t err;
        err = ::IOCreatePlugInInterfaceForService(device,
                                                kIOUSBDeviceUserClientTypeID,
                                                kIOCFPlugInInterfaceID,
                                                &plugin,
                                                &score);
        if (err == KERN_SUCCESS && plugin) {
            IOUSBDeviceInterface245** usb_interface = nullptr;
            err = (*plugin)->QueryInterface(plugin,
                                            CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID245),
                                            reinterpret_cast<LPVOID*>(&usb_interface));
            if (err == KERN_SUCCESS && usb_interface) {
                UInt16 PID = 0;
                UInt16 VID = 0;
                UInt32 LocationID = 0;
                err = (*usb_interface)->GetDeviceVendor(usb_interface, &VID);
                err = (*usb_interface)->GetDeviceProduct(usb_interface, &PID);
                err = (*usb_interface)->GetLocationID(usb_interface, &LocationID);
                XAMP_LOG_DEBUG("VID = {:x} PID = {:x} LocationID = {:x}", VID, PID, LocationID);
                io_string_t path;
                err = ::IORegistryEntryGetPath(device, kIOServicePlane, path);
                (*usb_interface)->Release(usb_interface);
                XAMP_LOG_DEBUG("Path: {}", path);
                usb_device.emplace_back(path);
            }
            (*plugin)->Release(plugin);
        }
        ::IOObjectRelease(device);
    }
    ::IOObjectRelease(iter);
    return usb_device;
}

std::vector<uint32_t> GetAvailableSampleRates(AudioDeviceID id) {
    constexpr AudioObjectPropertyAddress property = {
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

std::string GetDeviceUid(AudioDeviceID id) {
    constexpr AudioObjectPropertyAddress property = {
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
        return "";
    }
    return SysCFStringRefToUTF8(uid);
}

std::wstring GetDeviceName(AudioDeviceID id, AudioObjectPropertySelector selector) {
    CFStringRef cfname;

    AudioObjectPropertyAddress property = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    uint32_t data_size = sizeof(CFStringRef);
    property.mSelector = selector;
    auto result = ::AudioObjectGetPropertyData(id,
                                               &property,
                                               0,
                                               nullptr,
                                               &data_size,
                                               &cfname);
    if (result != noErr) {
        CoreAudioFailedLog(result);
        return L"";
    }

    return String::ToStdWString(SysCFStringRefToUTF8(cfname));
}

std::wstring GetPropertyName(AudioDeviceID id) {
    return GetDeviceName(id, kAudioObjectPropertyName);
}

AudioDeviceID GetAudioDeviceIdByUid(bool is_input, std::string const& device_id) {
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
        auto uid = SysUTF8ToCFStringRef(device_id);

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
    constexpr AudioObjectPropertyAddress property = {
        kAudioDevicePropertyStreams,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMaster
    };

    UInt32 data_size = 0;
    auto result = ::AudioObjectGetPropertyDataSize(id,
                                                   &property,
                                                   0,
                                                   nullptr,
                                                   &data_size);
    if (result != noErr) {
        CoreAudioFailedLog(result);
        return false;
    }
    return (data_size / sizeof(AudioStreamID)) > 0;
}

bool IsAutoHogMode() {
    UInt32 val = 0;
    UInt32 size = sizeof(val);
    constexpr AudioObjectPropertyAddress property = {
        kAudioHardwarePropertyHogModeIsAllowed,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    auto result = ::AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                 &property,
                                 0,
                                 nullptr,
                                 &size,
                                 &val);
    if (result != noErr) {
        CoreAudioFailedLog(result);
        return false;
    }
    return (val == 1);
}

bool SetAutoHogMode(bool enable) {
    UInt32 val = enable ? 1 : 0;
    constexpr AudioObjectPropertyAddress property = {
        kAudioHardwarePropertyHogModeIsAllowed,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    auto result = ::AudioObjectSetPropertyData(kAudioObjectSystemObject,
                               &property,
                               0,
                               nullptr,
                               sizeof(val),
                               &val);
    if (result != noErr) {
        CoreAudioFailedLog(result);
        return false;
    }
    return true;
}

void ReleaseHogMode(AudioDeviceID id) {
    constexpr AudioObjectPropertyAddress property = {
        kAudioDevicePropertyHogMode,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    pid_t hog_pid;
    uint32_t data_size = sizeof(hog_pid);
    auto result = ::AudioObjectGetPropertyData(id,
                                               &property,
                                               0,
                                               nullptr,
                                               &data_size,
                                               &hog_pid);
    if (result != noErr) {
        CoreAudioFailedLog(result);
        return;
    }
    auto current_pid = ::getpid();
    if (hog_pid != current_pid) {
        return;
    }
    hog_pid = -1;
    result = ::AudioObjectSetPropertyData(id,
                                          &property,
                                          0,
                                          nullptr,
                                          data_size,
                                          &hog_pid);
    if (result != noErr) {
        CoreAudioFailedLog(result);
    }
    XAMP_LOG_TRACE("Release hog mode device id:{}", id);
}

DeviceConnectType GetDeviceConnectType(AudioDeviceID id) {
    constexpr AudioObjectPropertyAddress property = {
        kAudioDevicePropertyTransportType,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    AudioDevicePropertyID val;
    uint32_t size = sizeof(val);

    auto result = ::AudioObjectGetPropertyData(id,
                                               &property,
                                               0,
                                               nullptr,
                                               &size,
                                               &val);
    if (result != noErr) {
        CoreAudioFailedLog(result);
        return DeviceConnectType::CONNECT_TYPE_UNKNOWN;
    }
    if (val == kAudioDeviceTransportTypeBuiltIn) {
        return DeviceConnectType::CONNECT_TYPE_BUILT_IN;
    } else if (val == kAudioDeviceTransportTypeUSB) {
        return DeviceConnectType::CONNECT_TYPE_USB;
    }
    return DeviceConnectType::CONNECT_TYPE_UNKNOWN;
}

void SetHogMode(AudioDeviceID id) {
    constexpr AudioObjectPropertyAddress property = {
        kAudioDevicePropertyHogMode,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    auto hog_pid = ::getpid();
    uint32_t dataSize = sizeof(hog_pid);
    auto result = ::AudioObjectSetPropertyData(id,
                                               &property,
                                               0,
                                               nullptr,
                                               dataSize,
                                               &hog_pid);
    if (result != noErr) {
        CoreAudioFailedLog(result);
    }

    XAMP_LOG_TRACE("Set hog mode id:{}", id);
}

}

#endif
