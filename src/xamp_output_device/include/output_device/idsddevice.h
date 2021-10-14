//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>
#include <base/base.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

using namespace base;

MAKE_ENUM(DsdIoFormat,
    IO_FORMAT_PCM,
    IO_FORMAT_DSD)

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IDsdDevice {
public:
    XAMP_BASE_CLASS(IDsdDevice)

    virtual void SetIoFormat(DsdIoFormat format) = 0;

    [[nodiscard]] virtual DsdIoFormat GetIoFormat() const = 0;

protected:
    IDsdDevice() = default;
};

}

