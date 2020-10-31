//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_MAC

#include <output_device/device_type.h>

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>

namespace xamp::output_device::osx {

using namespace base;

class XAMP_OUTPUT_DEVICE_API CoreAudioDeviceType : public DeviceType {
public:
    constexpr static auto Id = std::string_view("E6BB3BF2-F16A-489B-83EE-4A29755F42E4");

    CoreAudioDeviceType();

    void ScanNewDevice() override;

    std::string_view GetDescription() const override;

    Uuid GetTypeId() const override;

    AlignPtr<Device> MakeDevice(const std::string &device_id) override;

    size_t GetDeviceCount() const override;

    DeviceInfo GetDeviceInfo(uint32_t device) const override;

    std::vector<DeviceInfo> GetDeviceInfo() const override;

    std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;

private:
    std::vector<DeviceInfo> device_list_;
};

}

#endif
