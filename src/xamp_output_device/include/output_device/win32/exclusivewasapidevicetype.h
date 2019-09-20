#pragma once

//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/win32/wasapi.h>
#include <output_device/device_type.h>

namespace xamp::output_device::win32 {

using namespace base;

class XAMP_OUTPUT_DEVICE_API ExclusiveWasapiDeviceType final : public DeviceType{
public:
	static const ID Id;

	ExclusiveWasapiDeviceType();

	virtual ~ExclusiveWasapiDeviceType() = default;

	void ScanNewDevice() override;

	std::wstring GetName() const override;

	const ID& GetTypeId() const override;

	int32_t GetDeviceCount() const override;

	DeviceInfo GetDeviceInfo(int32_t device) const override;

	DeviceInfo GetDefaultOutputDeviceInfo() const override;

	std::vector<DeviceInfo> GetDeviceInfo() const override;

	std::unique_ptr<Device> MakeDevice(const std::wstring& device_id) override;

private:
	CComPtr<IMMDevice> GetDeviceById(const std::wstring& device_id) const;

	std::vector<DeviceInfo> GetDeviceInfoList() const;

	CComPtr<IMMDeviceEnumerator> enumerator_;
	std::vector<DeviceInfo> device_list_;
};

}
