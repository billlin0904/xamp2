//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/device_type.h>

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>

namespace xamp::output_device::osx {

using namespace base;

class XAMP_OUTPUT_DEVICE_API CoreAudioDeviceType final : public DeviceType {
public:
    static const ID Id;

    CoreAudioDeviceType();

    void ScanNewDevice() override;

    std::wstring GetDescription() const override;

    const ID &GetTypeId() const override;

    AlignPtr<Device> MakeDevice(const std::wstring &device_id) override;

    int32_t GetDeviceCount() const override;

    DeviceInfo GetDeviceInfo(int32_t device) const override;

    std::vector<DeviceInfo> GetDeviceInfo() const override;

    DeviceInfo GetDefaultDeviceInfo() const override;
private:
    std::vector<DeviceInfo> device_list_;
};

}
