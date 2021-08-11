//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/output_device.h>

namespace xamp::output_device {

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE DeviceStateNotification {
public:
	virtual ~DeviceStateNotification() = default;

	virtual void Run() = 0;

protected:
	DeviceStateNotification() = default;
};

}