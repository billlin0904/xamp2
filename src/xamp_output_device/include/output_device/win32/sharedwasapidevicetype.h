//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/align_ptr.h>
#include <output_device/win32/wasapi.h>
#include <output_device/device_type.h>

namespace xamp::output_device::win32 {

class XAMP_OUTPUT_DEVICE_API SharedWasapiDeviceType final : public DeviceType {
public:
	static std::string_view const Id;

	SharedWasapiDeviceType() noexcept;

	void ScanNewDevice() override;

	std::string_view GetDescription() const override;

	ID GetTypeId() const override;

	size_t GetDeviceCount() const override;

	DeviceInfo GetDeviceInfo(uint32_t device) const override;

	std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;

	std::vector<DeviceInfo> GetDeviceInfo() const override;

	AlignPtr<Device> MakeDevice(const std::wstring& device_id) override;
	
private:
	void Initial();

	std::vector<DeviceInfo> GetDeviceInfoList() const;

	CComPtr<IMMDevice> GetDeviceById(const std::wstring& device_id) const;

	std::vector<DeviceInfo> device_list_;
	CComPtr<IMMDeviceEnumerator> enumerator_;
};

}

#endif

