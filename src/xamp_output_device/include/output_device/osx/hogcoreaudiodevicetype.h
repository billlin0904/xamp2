//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_MAC

#include <output_device/idevicetype.h>
#include <output_device/osx/coreaudiodevicetype.h>

namespace xamp::output_device::osx {

class XAMP_OUTPUT_DEVICE_API HogCoreAudioDeviceType : public CoreAudioDeviceType {
    XAMP_DECLARE_MAKE_CLASS_UUID(HogCoreAudioDeviceType, "C9E867E1-0B9A-4507-8823-746B24D5FB3E")

public:
    constexpr static auto Description = std::string_view("CoreAudio (Hog Mode)");

    HogCoreAudioDeviceType();

    std::string_view GetDescription() const override;

    Uuid GetTypeId() const override;

    AlignPtr<IOutputDevice> MakeDevice(const std::string &device_id) override;
};

}

#endif

