//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#if ENABLE_ASIO
#include <base/dsdsampleformat.h>
#include <output_device/device.h>

namespace xamp::output_device {

using namespace base;

enum class AsioIoFormat {
	IO_FORMAT_DSD,
	IO_FORMAT_PCM
};

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE DSDDevice {
public:
    XAMP_BASE_CLASS(DSDDevice)

    virtual bool IsSupportDSDFormat() const = 0;

    virtual void SetIoFormat(AsioIoFormat format) = 0;

    virtual AsioIoFormat GetIoFormat() const = 0;

	virtual void SetSampleFormat(DsdSampleFormat format) = 0;

	virtual DsdSampleFormat GetSampleFormat() const = 0;

protected:
	DSDDevice() = default;
};

}
#endif
