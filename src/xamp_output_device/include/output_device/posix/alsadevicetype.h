//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_LINUX
#include <output_device/idevicetype.h>

namespace xamp::output_device::posix {

class AlsaDeviceType final : public IDeviceType {
public:
	constexpr static auto Id = std::string_view("8A530587-B9C4-4D04-81B4-7C4A3063F2EC");
	
	~AlsaDeviceType() override;
	
	void ScanNewDevice() override;
	
	std::string_view GetDescription() const override;
	
	Uuid GetTypeId() const override;
	
	AlignPtr<Device> MakeDevice(std::string const& device_id) override;
	
	size_t GetDeviceCount() const override;
	
	DeviceInfo GetDeviceInfo(uint32_t device) const override;
	
	std::vector<DeviceInfo> GetDeviceInfo() const override;
	
	std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;
};

}
#endif

