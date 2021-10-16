#include <output_device/osx/hogcoreaudiodevicetype.h>

#include <base/str_utilts.h>
#include <base/align_ptr.h>
#include <base/logger.h>

#include <output_device/osx/coreaudiodevice.h>
#include <output_device/osx/osx_utitl.h>

namespace xamp::output_device::osx {

HogCoreAudioDeviceType::HogCoreAudioDeviceType() {
}

std::string_view HogCoreAudioDeviceType::GetDescription() const {
    return "CoreAudio (Exclusive Mode)";
}

AlignPtr<IDevice> HogCoreAudioDeviceType::MakeDevice(const std::string &device_id) {
    auto id = GetAudioDeviceIdByUid(false, device_id);
    return MakeAlign<IDevice, CoreAudioDevice>(id, true);
}

Uuid HogCoreAudioDeviceType::GetTypeId() const {
    return Id;
}

}
