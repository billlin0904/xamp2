//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#if ENABLE_ASIO && XAMP_OS_WIN

#include <base/memory.h>
#include <base/pimplptr.h>
#include <base/stl.h>
#include <base/uuidof.h>

#include <output_device/output_device.h>
#include <output_device/idevicetype.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

/*
* AsioDeviceType is the asio device type.
* 
*/
class AsioDeviceType final : public IDeviceType {
	XAMP_DECLARE_MAKE_CLASS_UUID(AsioDeviceType, "0B3FF8BC-5BFD-4A08-8066-95974FB11BB5")

public:
	constexpr static auto Description = std::string_view("ASIO");

	/*
	* Constructor
	*/
	AsioDeviceType();

	/*
	* Destructor
	*/
	virtual ~AsioDeviceType() = default;
	
	/*
	* Get device type description
	* 
	* @return std::string_view
	*/
	std::string_view GetDescription() const override;

	/*
	* Get device type id
	* 
	* @return Uuid
	*/
	Uuid GetTypeId() const override;

	/*
	* Get device count
	* 
	* @return size_t
	*/
	size_t GetDeviceCount() const override;

	/*
	* Get device info
	* 
	* @param device: device index
	* @return DeviceInfo
	*/
    DeviceInfo GetDeviceInfo(uint32_t device) const override;

	/*
	* Get default device info
	* 
	* @return std::optional<DeviceInfo>
	*/
	std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;

	/*
	* Get device info
	* 
	* @return Vector<DeviceInfo>
	*/
	Vector<DeviceInfo> GetDeviceInfo() const override;

	/*
	* Scan new device
	* 
	*/
    void ScanNewDevice() override;

	/*
	* Make device
	* 
	* @param device_id: device id
	* @return IOutputDevice
	*/
	AlignPtr<IOutputDevice> MakeDevice(std::string const &device_id) override;
private:
	class AsioDeviceTypeImpl;
	PimplPtr<AsioDeviceTypeImpl> impl_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // ENABLE_ASIO && XAMP_OS_WIN
