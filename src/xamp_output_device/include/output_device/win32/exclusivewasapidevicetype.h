//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/idevicetype.h>

#include <base/uuidof.h>
#include <base/memory.h>
#include <base/memory.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

/*
* ExclusiveWasapiDeviceType is the device type for exclusive mode wasapi.
* 
*/
class ExclusiveWasapiDeviceType final : public IDeviceType {
	XAMP_DECLARE_MAKE_CLASS_UUID(ExclusiveWasapiDeviceType, "089F8446-C980-495B-AC80-5A437A4E73F6")

public:
	XAMP_DECLARE_UUID_CLASS_DESC(ExclusiveWasapiDeviceType, "Exclusive WASAPI")

	/*
	* Constructor
	*/
	ExclusiveWasapiDeviceType() ;

	XAMP_PIMPL(ExclusiveWasapiDeviceType)
	
	/*
	* Scan new device
	*/
	void ScanNewDevice() override;

	/*
	* Get device count
	* 
	* @return size_t
	*/
	[[nodiscard]] size_t GetDeviceCount() const override;

	/*
	* Get device info
	* 
	* @param device: device index
	* @return DeviceInfo
	*/
	[[nodiscard]] DeviceInfo GetDeviceInfo(uint32_t device) const override;

	/*
	* Get default device info
	* 
	* @return std::optional<DeviceInfo>	 
	*/
	[[nodiscard]] std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;

	/*
	* Get device info
	* 
	* @return std::vector<DeviceInfo>
	*/
	[[nodiscard]] std::vector<DeviceInfo> GetDeviceInfo() const override;

	/*
	* Make device
	* 
	* @param device_id: device id
	* @return ScopedPtr<IOutputDevice>
	*/
	ScopedPtr<IOutputDevice> MakeDevice(const std::shared_ptr<IThreadPoolExecutor>& thread_pool, const std::string & device_id) override;

private:
	class ExclusiveWasapiDeviceTypeImpl;	
	ScopedPtr<ExclusiveWasapiDeviceTypeImpl> impl_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif
