//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#if ENABLE_ASIO

#include <base/align_ptr.h>
#include <base/stl.h>

#include <output_device/output_device.h>
#include <output_device/idevicetype.h>

namespace xamp::output_device::win32 {

class ASIODeviceType final : public IDeviceType {
public:
	constexpr static auto Id = std::string_view("0B3FF8BC-5BFD-4A08-8066-95974FB11BB5");
	constexpr static auto Description = std::string_view("ASIO 2.0");

    ASIODeviceType();

	std::string_view GetDescription() const override;

	Uuid GetTypeId() const override;

	size_t GetDeviceCount() const override;

    DeviceInfo GetDeviceInfo(uint32_t device) const override;

	std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;

	std::vector<DeviceInfo> GetDeviceInfo() const override;

    void ScanNewDevice() override;

	AlignPtr<IOutputDevice> MakeDevice(std::string const &device_id) override;
private:
	DeviceInfo GetDeviceInfo(std::wstring const& name, std::string const & device_id) const;

	HashMap<std::string, DeviceInfo> device_list_;
};

}
#endif