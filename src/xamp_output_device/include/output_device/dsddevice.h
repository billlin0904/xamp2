//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/dsdsampleformat.h>
#include <output_device/device.h>

namespace xamp::output_device {

using namespace base;

enum class AsioIoFormat {
	IO_FORMAT_DSD,
	IO_FORMAT_PCM
};

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE DsdDevice {
public:
	virtual ~DsdDevice() = default;

    virtual bool IsSupportDsdFormat() const = 0;

    virtual void SetIoFormat(AsioIoFormat format) = 0;

    virtual AsioIoFormat GetIoFormat() const = 0;

	virtual void SetSampleFormat(DsdSampleFormat format) = 0;

	virtual DsdSampleFormat GetSampleFormat() const = 0;

protected:
	DsdDevice() = default;
};

}

