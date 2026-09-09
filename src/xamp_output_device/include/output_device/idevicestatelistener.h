//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/devicestate.h>
#include <output_device/output_device.h>

#include <base/base.h>

#include <string>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IDeviceStateListener {
public:
	XAMP_BASE_CLASS(IDeviceStateListener)

    virtual void onDeviceStateChange(DeviceState state, const std::string &device_id) = 0;

protected:
	IDeviceStateListener() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END
