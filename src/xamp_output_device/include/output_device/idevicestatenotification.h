//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/output_device.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IDeviceStateNotification {
public:
	XAMP_BASE_CLASS(IDeviceStateNotification)

	virtual void run() = 0;

protected:
	IDeviceStateNotification() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END