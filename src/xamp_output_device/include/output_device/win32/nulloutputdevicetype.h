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

namespace xamp::output_device::win32 {

XAMP_DECLARE_LOG_NAME(NullOutputDeviceType);

class NullOutputDeviceType final : public IDeviceType {
	XAMP_DECLARE_MAKE_CLASS_UUID(NullOutputDeviceType, "3B5452A1-7747-456E-80D4-E04929B05F66")

public:
	constexpr static auto Description = std::string_view("Null Output");

	NullOutputDeviceType() noexcept;

	void ScanNewDevice() override;

	[[nodiscard]] std::string_view GetDescription() const override;

	[[nodiscard]] Uuid GetTypeId() const override;

	[[nodiscard]] size_t GetDeviceCount() const override;

	[[nodiscard]] DeviceInfo GetDeviceInfo(uint32_t device) const override;

	[[nodiscard]] std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;

	[[nodiscard]] Vector<DeviceInfo> GetDeviceInfo() const override;

	AlignPtr<IOutputDevice> MakeDevice(const std::string& device_id) override;
	
private:
	class NullOutputDeviceTypeImpl;
	PimplPtr<NullOutputDeviceTypeImpl> impl_;
};

}

#endif


