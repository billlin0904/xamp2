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

class ExclusiveWasapiDeviceType final : public IDeviceType {
	XAMP_DECLARE_MAKE_CLASS_UUID(ExclusiveWasapiDeviceType, "089F8446-C980-495B-AC80-5A437A4E73F6")

public:
	XAMP_DECLARE_UUID_CLASS_DESC(ExclusiveWasapiDeviceType, "Exclusive WASAPI")

	ExclusiveWasapiDeviceType() ;

	XAMP_PIMPL(ExclusiveWasapiDeviceType)
	
	void scanNewDevice() override;

	[[nodiscard]] size_t getDeviceCount() const override;

	[[nodiscard]] DeviceInfo getDeviceInfo(uint32_t device) const override;

	[[nodiscard]] std::optional<DeviceInfo> getDefaultDeviceInfo() const override;

	[[nodiscard]] std::vector<DeviceInfo> getDeviceInfo() const override;

	ScopedPtr<IOutputDevice> makeDevice(const std::shared_ptr<IThreadPool>& thread_pool, const std::string & device_id) override;

private:
	class ExclusiveWasapiDeviceTypeImpl;	
	ScopedPtr<ExclusiveWasapiDeviceTypeImpl> impl_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif
