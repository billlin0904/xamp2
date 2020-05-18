//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <base/stl.h>

#include <output_device/win32/wasapi.h>
#include <output_device/device_type.h>

#ifdef _WIN32

namespace xamp::output_device::win32 {

using namespace base;

class XAMP_OUTPUT_DEVICE_API ExclusiveWasapiDeviceType final : public DeviceType {
public:
	static ID const Id;

	ExclusiveWasapiDeviceType();

	void ScanNewDevice() override;

	std::string_view GetDescription() const override;

	const ID& GetTypeId() const override;

	size_t GetDeviceCount() const override;

	DeviceInfo GetDeviceInfo(uint32_t device) const override;

	std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;

	std::vector<DeviceInfo> GetDeviceInfo() const override;

	AlignPtr<Device> MakeDevice(std::wstring const & device_id) override;

private:
	void Initial();

	CComPtr<IMMDevice> GetDeviceById(std::wstring const & device_id) const;

	std::vector<DeviceInfo> GetDeviceInfoList() const;

	CComPtr<IMMDeviceEnumerator> enumerator_;
	std::vector<DeviceInfo> device_list_;
};

}

#endif

