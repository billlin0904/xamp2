//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>
#include <base/base.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IVolumeLevel {
public:
    XAMP_BASE_CLASS(IVolumeLevel)

    virtual void SetVolumeLevel(float volume_db) = 0;
protected:
    IVolumeLevel() = default;
};

}

