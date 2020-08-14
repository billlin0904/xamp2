//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <optional>

#include <base/base.h>
#include <base/align_ptr.h>
#include <base/stl.h>

#include <output_device/deviceinfo.h>
#include <output_device/device.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

using namespace base;

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE DeviceType {
public:
	virtual ~DeviceType() = default;

	virtual void ScanNewDevice() = 0;

	virtual std::string_view GetDescription() const = 0;

    virtual ID GetTypeId() const = 0;

    virtual AlignPtr<Device> MakeDevice(std::string const & device_id) = 0;

	virtual size_t GetDeviceCount() const = 0;

    virtual DeviceInfo GetDeviceInfo(uint32_t device) const = 0;

	virtual std::vector<DeviceInfo> GetDeviceInfo() const = 0;

	virtual std::optional<DeviceInfo> GetDefaultDeviceInfo() const = 0;
protected:
	DeviceType() = default;
};

}
