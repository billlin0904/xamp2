//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>

#ifdef _WIN32
#include <output_device/win32/wasapi.h>
#include <output_device/device_type.h>

namespace xamp::output_device::win32 {

class XAMP_OUTPUT_DEVICE_API SharedWasapiDeviceType final : public DeviceType {
public:
	static const ID Id;

	SharedWasapiDeviceType();

	virtual ~SharedWasapiDeviceType() = default;

	void ScanNewDevice() override;

	std::string_view GetDescription() const override;

	const ID& GetTypeId() const override;

	int32_t GetDeviceCount() const override;

	DeviceInfo GetDeviceInfo(int32_t device) const override;

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

