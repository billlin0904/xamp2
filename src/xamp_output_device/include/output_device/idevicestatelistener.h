//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/base.h>
#include <output_device/devicestate.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IDeviceStateListener {
public:
	XAMP_BASE_CLASS(IDeviceStateListener)

    virtual void OnDeviceStateChange(DeviceState state, std::string const &device_id) = 0;

protected:
	IDeviceStateListener() = default;
};

}
