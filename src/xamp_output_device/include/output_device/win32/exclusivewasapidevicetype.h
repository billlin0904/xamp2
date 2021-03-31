//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/align_ptr.h>
#include <base/stl.h>

#include <output_device/win32/wasapi.h>
#include <output_device/device_type.h>

namespace xamp::output_device::win32 {

using namespace base;

class ExclusiveWasapiDeviceType final : public DeviceType {
public:
	constexpr static auto Id = std::string_view("089F8446-C980-495B-AC80-5A437A4E73F6");

	ExclusiveWasapiDeviceType() noexcept;

	void ScanNewDevice() override;

	[[nodiscard]] std::string_view GetDescription() const override;

	[[nodiscard]] Uuid GetTypeId() const override;

	[[nodiscard]] size_t GetDeviceCount() const override;

	[[nodiscard]] DeviceInfo GetDeviceInfo(uint32_t device) const override;

	[[nodiscard]] std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;

	[[nodiscard]] std::vector<DeviceInfo> GetDeviceInfo() const override;

	AlignPtr<Device> MakeDevice(std::string const & device_id) override;

private:
	void Initial();

	[[nodiscard]] CComPtr<IMMDevice> GetDeviceById(std::wstring const & device_id) const;

	[[nodiscard]] std::vector<DeviceInfo> GetDeviceInfoList() const;

	CComPtr<IMMDeviceEnumerator> enumerator_;
	std::vector<DeviceInfo> device_list_;
};

}

#endif

