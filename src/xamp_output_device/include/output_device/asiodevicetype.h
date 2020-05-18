//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#if ENABLE_ASIO

#include <base/base.h>
#include <base/align_ptr.h>
#include <base/stl.h>
#include <output_device/device_type.h>

namespace xamp::output_device {

class XAMP_OUTPUT_DEVICE_API ASIODeviceType final : public DeviceType {
public:
	static ID const Id;

    ASIODeviceType();

	std::string_view GetDescription() const override;

	ID const & GetTypeId() const override;

	size_t GetDeviceCount() const override;

    DeviceInfo GetDeviceInfo(uint32_t device) const override;

	std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;

	std::vector<DeviceInfo> GetDeviceInfo() const override;

    void ScanNewDevice() override;

	AlignPtr<Device> MakeDevice(std::wstring const &device_id) override;
private:
	DeviceInfo GetDeviceInfo(std::wstring const & device_id) const;

	RobinHoodHashMap<std::wstring, DeviceInfo> device_list_;
};

}
#endif
