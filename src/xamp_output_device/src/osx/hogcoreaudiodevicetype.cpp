#include <output_device/osx/hogcoreaudiodevicetype.h>

#if 1
#include <base/str_utilts.h>
#include <base/memory.h>
#include <base/logger.h>

#include <output_device/osx/coreaudiodevice.h>
#include <output_device/osx/osx_utitl.h>

namespace xamp::output_device::osx {

HogCoreAudioDeviceType::HogCoreAudioDeviceType() {
}

std::string_view HogCoreAudioDeviceType::getDescription() const {
    return Description;
}

ScopedPtr<IOutputDevice> HogCoreAudioDeviceType::makeDevice(const std::shared_ptr<IThreadPool>& thread_pool, const std::string &device_id) {
    auto id = GetAudioDeviceIdByUid(false, device_id);
    return makeAlign<IOutputDevice, CoreAudioDevice>(id, true);
}

Uuid HogCoreAudioDeviceType::getTypeId() const {
    return XAMP_UUID_OF(HogCoreAudioDeviceType);
}

}
#endif
