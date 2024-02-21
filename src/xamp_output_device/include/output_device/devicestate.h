//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>
#include <output_device/output_device.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

/*
* DeviceState is the enum for device state.
* 
* <remarks>
* DEVICE_STATE_ADDED: Device added.
* DEVICE_STATE_REMOVED: Device removed.
* DEVICE_STATE_DEFAULT_DEVICE_CHANGE: Default device change.
* </remarks>
*/
XAMP_MAKE_ENUM(DeviceState,
          DEVICE_STATE_ADDED,
          DEVICE_STATE_REMOVED,
          DEVICE_STATE_DEFAULT_DEVICE_CHANGE)

XAMP_OUTPUT_DEVICE_NAMESPACE_END
