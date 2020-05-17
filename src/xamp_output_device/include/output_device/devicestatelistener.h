//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/base.h>
#include <base/enum.h>
#include <output_device/devicestate.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

using namespace base;

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE DeviceStateListener {
public:
	virtual ~DeviceStateListener() = default;

    virtual void OnDeviceStateChange(DeviceState state, std::wstring const &device_id) = 0;

protected:
	DeviceStateListener() = default;
};

}
