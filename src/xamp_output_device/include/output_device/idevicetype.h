//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
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
#include <output_device/idevice.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

using namespace base;

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IDeviceType {
public:
	XAMP_BASE_CLASS(IDeviceType)

	virtual void ScanNewDevice() = 0;

	[[nodiscard]] virtual std::string_view GetDescription() const = 0;

	[[nodiscard]] virtual Uuid GetTypeId() const = 0;

    virtual AlignPtr<IDevice> MakeDevice(std::string const & device_id) = 0;

	[[nodiscard]] virtual size_t GetDeviceCount() const = 0;

	[[nodiscard]] virtual DeviceInfo GetDeviceInfo(uint32_t device) const = 0;

	[[nodiscard]] virtual std::vector<DeviceInfo> GetDeviceInfo() const = 0;

	[[nodiscard]] virtual std::optional<DeviceInfo> GetDefaultDeviceInfo() const = 0;

protected:
	IDeviceType() = default;
};

}