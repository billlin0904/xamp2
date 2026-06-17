//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/logger.h>
#include <base/memory.h>
#include <base/uuidof.h>

#include <output_device/idevicetype.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(NullOutputDeviceType);

/*
* NullOutputDeviceType is null output device type.
*
*/
class NullOutputDeviceType final : public IDeviceType {
	XAMP_DECLARE_MAKE_CLASS_UUID(NullOutputDeviceType, "6F1223E0-231A-495C-B16A-2AAC851F8D5F")

public:
	XAMP_DECLARE_UUID_CLASS_DESC(NullOutputDeviceType, "Null Device")

	/*
	* Constructor.
	*/
	NullOutputDeviceType();

	/*
	* Destructor.
	*/
	virtual ~NullOutputDeviceType() override = default;

	/*
	* Scan new device.
	*/
	void scanNewDevice() override;

	/*
	* Get device count
	*
	* @return size_t
	*/
	[[nodiscard]] size_t getDeviceCount() const override;

	/*
	* Get device info
	*
	* @param device: device index
	* @return DeviceInfo
	*/
	[[nodiscard]] DeviceInfo getDeviceInfo(uint32_t device) const override;

	/*
	* Get default device info
	*
	* @return std::optional<DeviceInfo>
	*/
	[[nodiscard]] std::optional<DeviceInfo> getDefaultDeviceInfo() const override;

	/*
	* Get device info
	*
	* @return std::vector<DeviceInfo>
	*/
	[[nodiscard]] std::vector<DeviceInfo> getDeviceInfo() const override;

	/*
	* Make device
	*
	* @param device_id: device id
	* @return ScopedPtr<IOutputDevice>
	*/
	ScopedPtr<IOutputDevice> makeDevice(const std::shared_ptr<IThreadPool>& thread_pool, const std::string& device_id) override;

private:
	class NullOutputDeviceTypeImpl;
	ScopedPtr<NullOutputDeviceTypeImpl> impl_;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END
