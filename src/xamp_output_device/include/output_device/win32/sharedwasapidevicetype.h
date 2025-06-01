//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/uuidof.h>
#include <base/memory.h>
#include <base/pimplptr.h>
#include <base/logger.h>

#include <output_device/idevicetype.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

/*
 * SharedWasapiDeviceType is the device type for shared mode wasapi.
 */
class XAMP_OUTPUT_DEVICE_API SharedWasapiDeviceType final : public IDeviceType {
	XAMP_DECLARE_MAKE_CLASS_UUID(SharedWasapiDeviceType, "07885EDF-7CCB-4FA6-962D-B66A759978B1")

public:
	static constexpr auto Description = std::string_view("WASAPI (Shared Mode)");

	/*
	 * Constructor
	 */
	SharedWasapiDeviceType();

	XAMP_PIMPL(SharedWasapiDeviceType)

	/*
	* Scan new device
	*/
	void ScanNewDevice() override;

	/*
	* Get device description
	*
	* @return std::string_view
	*/
	[[nodiscard]] std::string_view GetDescription() const override;

	/*
	* Get device type id
	*
	* @return Uuid
	*/
	[[nodiscard]] Uuid GetTypeId() const override;

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
	ScopedPtr<IOutputDevice> MakeDevice(const std::shared_ptr<IThreadPoolExecutor>& thread_pool, const std::string& device_id) override;
	
private:
	class SharedWasapiDeviceTypeImpl;
	ScopedPtr<SharedWasapiDeviceTypeImpl> impl_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // XAMP_OS_WIN

