//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <optional>
#include <string>

#include <base/uuid.h>
#include <base/enum.h>
#include <output_device/output_device.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

/*
* DeviceConnectType is the enum for device connect type.
* 
* <remarks>
* UKNOWN: Unknown.
* USB: USB.
* BUILT_IN: Built in.
* BLUE_TOOTH: Blue tooth.
* </remarks>
*/
XAMP_MAKE_ENUM(DeviceConnectType,
	UKNOWN, 
	USB, 
	BUILT_IN,
	BLUE_TOOTH)

/*
* DeviceInfo is the device info.
* 
*/
struct XAMP_OUTPUT_DEVICE_API DeviceInfo final {
	bool is_default_device{ false };
	bool is_support_dsd{ false };
	bool is_hardware_control_volume{ false };
	DeviceConnectType connect_type{ DeviceConnectType::UKNOWN };
	std::wstring name;    
	std::string device_id;
	Uuid device_type_id;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END
