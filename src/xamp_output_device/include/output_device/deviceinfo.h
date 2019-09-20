//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/id.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

using namespace base;

struct XAMP_OUTPUT_DEVICE_API DeviceInfo final {
	DeviceInfo()
		: is_default_device(false) {
	}

	~DeviceInfo() noexcept = default;
	bool is_default_device;
	std::wstring name;    
	std::wstring device_id;
	ID device_type_id;
};

}
