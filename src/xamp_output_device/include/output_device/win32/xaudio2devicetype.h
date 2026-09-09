//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/memory.h>
#include <base/memory.h>
#include <base/stl.h>
#include <base/uuidof.h>
#include <base/logger.h>

#include <output_device/output_device.h>
#include <output_device/idevicetype.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(XAudio2DeviceType);

class XAudio2DeviceType final : public IDeviceType {
	XAMP_DECLARE_MAKE_CLASS_UUID(XAudio2DeviceType, "3B5452A1-7747-456E-80D4-E04929B05F66")

public:
	XAMP_DECLARE_UUID_CLASS_DESC(XAudio2DeviceType, "XAudio2")

	XAudio2DeviceType();

	XAMP_PIMPL(XAudio2DeviceType)

	size_t getDeviceCount() const override;

	DeviceInfo getDeviceInfo(uint32_t device) const override;

	std::optional<DeviceInfo> getDefaultDeviceInfo() const override;

	std::vector<DeviceInfo> getDeviceInfo() const override;

	void scanNewDevice() override;

	ScopedPtr<IOutputDevice> makeDevice(const std::shared_ptr<IThreadPool>& thread_pool, std::string const& device_id) override;
private:
	class XAudio2DeviceTypeImpl;
	ScopedPtr<XAudio2DeviceTypeImpl> impl_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END
