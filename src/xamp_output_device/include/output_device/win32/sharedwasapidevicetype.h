//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/uuidof.h>
#include <base/memory.h>
#include <base/memory.h>
#include <base/logger.h>

#include <output_device/idevicetype.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

class XAMP_OUTPUT_DEVICE_API SharedWasapiDeviceType final : public IDeviceType {
	XAMP_DECLARE_MAKE_CLASS_UUID(SharedWasapiDeviceType, "07885EDF-7CCB-4FA6-962D-B66A759978B1")

public:
	XAMP_DECLARE_UUID_CLASS_DESC(SharedWasapiDeviceType, "Shared WASAPI")

	SharedWasapiDeviceType();

	XAMP_PIMPL(SharedWasapiDeviceType)

	void scanNewDevice() override;

	[[nodiscard]] size_t getDeviceCount() const override;

	[[nodiscard]] DeviceInfo getDeviceInfo(uint32_t device) const override;

	[[nodiscard]] std::optional<DeviceInfo> getDefaultDeviceInfo() const override;

	[[nodiscard]] std::vector<DeviceInfo> getDeviceInfo() const override;

	ScopedPtr<IOutputDevice> makeDevice(const std::shared_ptr<IThreadPool>& thread_pool, const std::string& device_id) override;
	
private:
	class SharedWasapiDeviceTypeImpl;
	ScopedPtr<SharedWasapiDeviceTypeImpl> impl_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // XAMP_OS_WIN

