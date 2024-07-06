//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/deviceinfo.h>
#include <output_device/ioutputdevice.h>
#include <output_device/output_device.h>

#include <base/base.h>
#include <base/memory.h>
#include <base/stl.h>

#include <string>
#include <memory>
#include <vector>
#include <optional>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

/*
* IDeviceType is the interface for device type.
* 
*/
class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IDeviceType {
public:
	XAMP_BASE_CLASS(IDeviceType)

	/*
	* Scan new device.
	* 
	*/
	virtual void ScanNewDevice() = 0;

	/*
	* Get device description.
	* 
	* @return std::string_view
	*/
	XAMP_NO_DISCARD virtual std::string_view GetDescription() const = 0;

	/*
	* Get device type id.
	* 
	* @return type id.
	*/
	XAMP_NO_DISCARD virtual Uuid GetTypeId() const = 0;

	/*
	* Make device.
	* 
	* @param device_id: device id.
	* 
	* @return AlignPtr<IOutputDevice>
	*/
    virtual AlignPtr<IOutputDevice> MakeDevice(const std::string & device_id) = 0;

	/*
	* Get device count.
	* 
	* @return size_t
	*/
	XAMP_NO_DISCARD virtual size_t GetDeviceCount() const = 0;

	/*
	* Get device info.
	*
	* @param device: device index.
	* 
	* @return DeviceInfo
	*/
	XAMP_NO_DISCARD virtual DeviceInfo GetDeviceInfo(uint32_t device) const = 0;

	/*
	* Get device info.
	* 
	* @return Vector<DeviceInfo>.
	*/
	XAMP_NO_DISCARD virtual Vector<DeviceInfo> GetDeviceInfo() const = 0;

	/*
	* Get default device info.
	* 
	* @return std::optional<DeviceInfo>
	*/
	XAMP_NO_DISCARD virtual std::optional<DeviceInfo> GetDefaultDeviceInfo() const = 0;	

protected:
	/*
	* Constructor.
	*/
	IDeviceType() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END