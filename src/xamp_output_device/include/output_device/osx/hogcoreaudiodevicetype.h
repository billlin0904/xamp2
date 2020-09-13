//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_MAC

#include <output_device/device_type.h>
#include <output_device/osx/coreaudiodevicetype.h>

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>

namespace xamp::output_device::osx {

using namespace base;

class XAMP_OUTPUT_DEVICE_API HogCoreAudioDeviceType : public CoreAudioDeviceType {
public:
    static const std::string_view Id;

    HogCoreAudioDeviceType();

    std::string_view GetDescription() const override;

    ID GetTypeId() const override;

    AlignPtr<Device> MakeDevice(const std::string &device_id) override;
};

}

#endif

