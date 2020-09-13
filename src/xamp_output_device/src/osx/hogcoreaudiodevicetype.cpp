#include <output_device/osx/hogcoreaudiodevicetype.h>

#include <base/str_utilts.h>
#include <base/align_ptr.h>
#include <base/logger.h>

#include <output_device/osx/coreaudiodevice.h>
#include <output_device/osx/osx_utitl.h>

namespace xamp::output_device::osx {

const std::string_view HogCoreAudioDeviceType::Id("44ED0EC0-069E-431F-8BF2-AB1369E3421F");

HogCoreAudioDeviceType::HogCoreAudioDeviceType() {
}

std::string_view HogCoreAudioDeviceType::GetDescription() const {
    return "CoreAudio (HogMode)";
}

AlignPtr<Device> HogCoreAudioDeviceType::MakeDevice(const std::string &device_id) {
    auto id = GetAudioDeviceIdByUid(false, device_id);
    return MakeAlign<Device, CoreAudioDevice>(id, true);
}

ID HogCoreAudioDeviceType::GetTypeId() const {
    return Id;
}

}
