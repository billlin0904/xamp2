//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/idevicetype.h>

#include <base/uuidof.h>
#include <base/align_ptr.h>
#include <base/pimplptr.h>
#include <base/logger.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

/*
 * SharedWasapiDeviceType is the device type for shared mode wasapi.
 */
class SharedWasapiDeviceType final : public IDeviceType {
	XAMP_DECLARE_MAKE_CLASS_UUID(SharedWasapiDeviceType, "07885EDF-7CCB-4FA6-962D-B66A759978B1")

public:
	constexpr static auto Description = std::string_view("WASAPI (Shared Mode)");

	/*
	 * Constructor
	 */
	SharedWasapiDeviceType() noexcept;

	/*
	 * Destructor
	 */
	virtual ~SharedWasapiDeviceType() = default;

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
	* @return Vector<DeviceInfo>
	*/
	[[nodiscard]] Vector<DeviceInfo> GetDeviceInfo() const override;

	/*
	* Make device
	*
	* @param device_id: device id
	* @return AlignPtr<IOutputDevice>
	*/
	AlignPtr<IOutputDevice> MakeDevice(const std::string& device_id) override;
	
private:
	class SharedWasapiDeviceTypeImpl;
	PimplPtr<SharedWasapiDeviceTypeImpl> impl_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // XAMP_OS_WIN

