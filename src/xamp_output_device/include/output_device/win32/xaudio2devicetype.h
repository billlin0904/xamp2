//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/memory.h>
#include <base/pimplptr.h>
#include <base/stl.h>
#include <base/uuidof.h>
#include <base/logger.h>

#include <output_device/output_device.h>
#include <output_device/idevicetype.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(XAudio2DeviceType);

class XAudio2DeviceType final : public IDeviceType {
	XAMP_DECLARE_MAKE_CLASS_UUID(XAudio2DeviceType, "6F1223E0-231A-495C-B16A-2AAC851F8D5F")

public:
	constexpr static auto Description = std::string_view("XAudio2");

	/*
	* Constructor
	*/
	XAudio2DeviceType();

	XAMP_PIMPL(XAudio2DeviceType)

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
	AlignPtr<IOutputDevice> MakeDevice(std::string const& device_id) override;
private:
	class XAudio2DeviceTypeImpl;
	AlignPtr<XAudio2DeviceTypeImpl> impl_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END
