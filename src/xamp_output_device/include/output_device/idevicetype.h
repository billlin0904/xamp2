//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/deviceinfo.h>
#include <output_device/ioutputdevice.h>
#include <output_device/output_device.h>

#include <base/base.h>
#include <base/memory.h>
#include <base/stl.h>
#include <base/uuid_class.h>

#include <string>
#include <memory>
#include <vector>
#include <optional>

XAMP_BASE_NAMESPACE_BEGIN
class IThreadPool;
XAMP_BASE_NAMESPACE_END

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IDeviceType : public IUUIDClass {
public:
	XAMP_BASE_CLASS(IDeviceType)

	virtual void scanNewDevice() = 0;

    virtual ScopedPtr<IOutputDevice> makeDevice(const std::shared_ptr<IThreadPool>& thread_pool, const std::string & device_id) = 0;

	[[nodiscard]] virtual size_t getDeviceCount() const = 0;

	[[nodiscard]] virtual DeviceInfo getDeviceInfo(uint32_t device) const = 0;

	[[nodiscard]] virtual std::vector<DeviceInfo> getDeviceInfo() const = 0;

	[[nodiscard]] virtual std::optional<DeviceInfo> getDefaultDeviceInfo() const = 0;	

protected:
	IDeviceType() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END