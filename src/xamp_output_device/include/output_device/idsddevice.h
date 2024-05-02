//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/output_device.h>

#include <base/enum.h>
#include <base/base.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

/*
* DsdIoFormat is the enum for DSD IO format.
* 
* <remarks>
* IO_FORMAT_PCM: PCM format.
* IO_FORMAT_DSD: DSD format.
* </remarks>
*/
XAMP_MAKE_ENUM(DsdIoFormat,
    IO_FORMAT_PCM,    
    IO_FORMAT_DSD,
    IO_FORMAT_DOP)

/*
* IDsdDevice is the interface for DSD device.
* 
*/
class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IDsdDevice {
public:
    XAMP_BASE_CLASS(IDsdDevice)

    /*
    * Set DSD IO format
    * 
    * @param format: DSD IO format
    */
    virtual void SetIoFormat(DsdIoFormat format) = 0;

    /*
    * Get DSD IO format
    * 
    * @return DsdIoFormat
    */
    [[nodiscard]] virtual DsdIoFormat GetIoFormat() const = 0;

protected:
    /*
    * Constructor
    */
    IDsdDevice() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END
