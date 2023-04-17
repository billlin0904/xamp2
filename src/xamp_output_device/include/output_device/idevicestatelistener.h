//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/devicestate.h>
#include <output_device/output_device.h>

#include <base/base.h>

#include <string>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

/*
* IDeviceStateListener is a callback interface for device state.
* 
*/
class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IDeviceStateListener {
public:
	XAMP_BASE_CLASS(IDeviceStateListener)

	/*
	* OnDeviceStateChange is called when device state changed.
	* 
	* @param[in] state is a device state.
	* @param[in] device_id is a device id.
	*/
    virtual void OnDeviceStateChange(DeviceState state, const std::string &device_id) = 0;

protected:
	/*
	* Constructor.
	*/
	IDeviceStateListener() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END
