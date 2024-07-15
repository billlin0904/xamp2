//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#if 1

#include <output_device/idevicetype.h>

#include <base/uuidof.h>
#include <base/memory.h>
#include <base/pimplptr.h>
#include <base/logger.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(NullOutputDeviceType);

/*
* NullOutputDeviceType is null output device type.
* 
*/
class NullOutputDeviceType final : public IDeviceType {
	XAMP_DECLARE_MAKE_CLASS_UUID(NullOutputDeviceType, "6F1223E0-231A-495C-B16A-2AAC851F8D5F")

public:
	constexpr static auto Description = std::string_view("Null Output");

	/*
	* Constructor.
	*/
	NullOutputDeviceType() noexcept;

	/*
	* Destructor.
	*/
	virtual ~NullOutputDeviceType() override = default;

	/*
	* Scan new device.
	*/
	void ScanNewDevice() override;

	/*
	* Get device description
	*
	* @return std::string_view
	*/
	XAMP_NO_DISCARD std::string_view GetDescription() const override;

	/*
	* Get device type id
	*
	* @return Uuid
	*/
	XAMP_NO_DISCARD Uuid GetTypeId() const override;

	/*
	* Get device count
	*
	* @return size_t
	*/
	XAMP_NO_DISCARD size_t GetDeviceCount() const override;

	/*
	* Get device info
	*
	* @param device: device index
	* @return DeviceInfo
	*/
	XAMP_NO_DISCARD DeviceInfo GetDeviceInfo(uint32_t device) const override;

	/*
	* Get default device info
	*
	* @return std::optional<DeviceInfo>
	*/
	XAMP_NO_DISCARD std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;

	/*
	* Get device info
	*
	* @return Vector<DeviceInfo>
	*/
	XAMP_NO_DISCARD Vector<DeviceInfo> GetDeviceInfo() const override;

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


