//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <output_device/output_device.h>

namespace xamp::output_device {

enum class DeviceState {
	DEVICE_STATE_ADDED,
	DEVICE_STATE_REMOVED,
	DEVICE_STATE_DEFAULT_DEVICE_CHANGE,
	_MAX_DEVICE_STATE_
};

class XAMP_OUTPUT_DEVICE_API DeviceStateListener {
public:
	XAMP_BASE_CLASS(DeviceStateListener)

	virtual void OnDeviceStateChange(DeviceState state, const std::wstring &device_id) = 0;

protected:
	DeviceStateListener() = default;
};

}
