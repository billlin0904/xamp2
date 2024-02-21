//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_MAC

#include <output_device/idevicetype.h>
#include <output_device/osx/coreaudiodevicetype.h>

#if 0

namespace xamp::output_device::osx {

class XAMP_OUTPUT_DEVICE_API HogCoreAudioDeviceType : public CoreAudioDeviceType {
    XAMP_DECLARE_MAKE_CLASS_UUID(HogCoreAudioDeviceType, "44ED0EC0-069E-431F-8BF2-AB1369E3421F")

public:
    constexpr static auto Description = std::string_view("CoreAudio (Hog Mode)");

    HogCoreAudioDeviceType();

    std::string_view GetDescription() const override;

    Uuid GetTypeId() const override;

    AlignPtr<IOutputDevice> MakeDevice(const std::string &device_id) override;
};

}

#endif

#endif

