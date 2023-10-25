//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/output_device.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

/*
* IDeviceStateNotification is the interface for device state notification.
* 
*/
class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IDeviceStateNotification {
public:
	XAMP_BASE_CLASS(IDeviceStateNotification)

	/*
	* Run.
	*/
	virtual void Run() = 0;

protected:
	/*
	* Constructor.
	*/
	IDeviceStateNotification() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END