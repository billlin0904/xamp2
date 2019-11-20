//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <optional>

#include <base/base.h>
#include <base/align_ptr.h>

#include <output_device/deviceinfo.h>
#include <output_device/device.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

using namespace base;

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE DeviceType {
public:
	XAMP_DISABLE_COPY(DeviceType)

	virtual ~DeviceType() = default;

	virtual void ScanNewDevice() = 0;

	virtual std::string_view GetDescription() const = 0;

	virtual const ID& GetTypeId() const = 0;

	virtual AlignPtr<Device> MakeDevice(const std::wstring& device_id) = 0;

	virtual int32_t GetDeviceCount() const = 0;

	virtual DeviceInfo GetDeviceInfo(int32_t device) const = 0;

	virtual std::vector<DeviceInfo> GetDeviceInfo() const = 0;

	virtual std::optional<DeviceInfo> GetDefaultDeviceInfo() const = 0;
protected:
	DeviceType() = default;
};

}