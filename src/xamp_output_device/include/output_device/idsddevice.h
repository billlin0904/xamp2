//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/output_device.h>

#include <base/enum.h>
#include <base/base.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(DsdIoFormat,
    IO_FORMAT_PCM,    
    IO_FORMAT_DSD,
    IO_FORMAT_DOP)

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IDsdDevice {
public:
    XAMP_BASE_CLASS(IDsdDevice)

    virtual void setIoFormat(DsdIoFormat format) = 0;

    [[nodiscard]] virtual DsdIoFormat getIoFormat() const = 0;

protected:
    IDsdDevice() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END
