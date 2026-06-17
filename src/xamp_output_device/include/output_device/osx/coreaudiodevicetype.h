//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_MAC

#include <base/uuidof.h>
#include <output_device/idevicetype.h>
#include <base/memory.h>
#include <base/memory.h>

namespace xamp::output_device::osx {

class XAMP_OUTPUT_DEVICE_API CoreAudioDeviceType : public IDeviceType {
    XAMP_DECLARE_MAKE_CLASS_UUID(CoreAudioDeviceType, "85C5C191-9103-4B94-AEAF-3D87DB54E864")

public:
    constexpr static auto Description = std::string_view("CoreAudio");

    XAMP_PIMPL(CoreAudioDeviceType)

    CoreAudioDeviceType();

    void scanNewDevice() override;

    std::string_view getDescription() const override;

    Uuid getTypeId() const override;

    AlignPtr<IOutputDevice> makeDevice(const std::string &device_id) override;

    size_t getDeviceCount() const override;

    DeviceInfo getDeviceInfo(uint32_t device) const override;

    Vector<DeviceInfo> getDeviceInfo() const override;

    std::optional<DeviceInfo> getDefaultDeviceInfo() const override;

protected:
    class CoreAudioDeviceTypeImpl;
    AlignPtr<CoreAudioDeviceTypeImpl> impl_;
};

}

#endif
