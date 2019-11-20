//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#if ENABLE_ASIO
#include <unordered_map>

#include <base/base.h>
#include <base/align_ptr.h>

#include <output_device/device_type.h>

namespace xamp::output_device {

class XAMP_OUTPUT_DEVICE_API ASIODeviceType final : public DeviceType {
public:
	static const ID Id;

    ASIODeviceType();

    virtual ~ASIODeviceType() = default;

	std::string_view GetDescription() const override;

	const ID& GetTypeId() const override;

    int32_t GetDeviceCount() const override;

    DeviceInfo GetDeviceInfo(int32_t device) const override;

	std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;

    std::vector<DeviceInfo> GetDeviceInfo() const override;

    void ScanNewDevice() override;

	AlignPtr<Device> MakeDevice(const std::wstring &device_id) override;
private:
	DeviceInfo GetDeviceInfo(const std::wstring& device_id) const;

	std::unordered_map<std::wstring, DeviceInfo> device_list_;
};

}
#endif
