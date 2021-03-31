//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/align_ptr.h>
#include <output_device/win32/wasapi.h>
#include <output_device/device_type.h>

namespace xamp::output_device::win32 {

class SharedWasapiDeviceType final : public DeviceType {
public:
	constexpr static auto Id = std::string_view("07885EDF-7CCB-4FA6-962D-B66A759978B1");

	SharedWasapiDeviceType() noexcept;

	void ScanNewDevice() override;

	[[nodiscard]] std::string_view GetDescription() const override;

	[[nodiscard]] Uuid GetTypeId() const override;

	[[nodiscard]] size_t GetDeviceCount() const override;

	[[nodiscard]] DeviceInfo GetDeviceInfo(uint32_t device) const override;

	[[nodiscard]] std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;

	[[nodiscard]] std::vector<DeviceInfo> GetDeviceInfo() const override;

	AlignPtr<Device> MakeDevice(const std::string& device_id) override;
	
private:
	void Initial();

	[[nodiscard]] std::vector<DeviceInfo> GetDeviceInfoList() const;

	[[nodiscard]] CComPtr<IMMDevice> GetDeviceById(const std::wstring& device_id) const;

	std::vector<DeviceInfo> device_list_;
	CComPtr<IMMDeviceEnumerator> enumerator_;
};

}

#endif

