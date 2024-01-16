//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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

XAMP_DECLARE_LOG_NAME(NullOutputDeviceType);

/*
* NullOutputDeviceType is null output device type.
* 
*/
class NullOutputDeviceType final : public IDeviceType {
	XAMP_DECLARE_MAKE_CLASS_UUID(NullOutputDeviceType, "3B5452A1-7747-456E-80D4-E04929B05F66")

public:
	constexpr static auto Description = std::string_view("Null Output");

	/*
	* Constructor.
	*/
	NullOutputDeviceType() noexcept;

	/*
	* Destructor.
	*/
	virtual ~NullOutputDeviceType() = default;

	/*
	* Scan new device.
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
	class NullOutputDeviceTypeImpl;
	AlignPtr<NullOutputDeviceTypeImpl> impl_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif


