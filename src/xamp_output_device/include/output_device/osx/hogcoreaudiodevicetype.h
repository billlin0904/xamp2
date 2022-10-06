//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_MAC

#include <output_device/idevicetype.h>
#include <output_device/osx/coreaudiodevicetype.h>

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>

namespace xamp::output_device::osx {

using namespace base;

class XAMP_OUTPUT_DEVICE_API HogCoreAudioDeviceType : public CoreAudioDeviceType {
public:
    constexpr static auto Id = std::string_view("44ED0EC0-069E-431F-8BF2-AB1369E3421F");
    constexpr static auto Description = std::string_view("CoreAudio (Hog Mode)");

    HogCoreAudioDeviceType();

    std::string_view GetDescription() const override;

    Uuid GetTypeId() const override;

    AlignPtr<IOutputDevice> MakeDevice(const std::string &device_id) override;
};

}

#endif

