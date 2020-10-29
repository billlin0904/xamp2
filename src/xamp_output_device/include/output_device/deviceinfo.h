//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/Uuid.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

using namespace base;

struct XAMP_OUTPUT_DEVICE_API DeviceInfo final {
	bool is_default_device{false};
	bool is_support_dsd{false};
	std::wstring name;    
	std::string device_id;
	Uuid device_type_id;
};

}
