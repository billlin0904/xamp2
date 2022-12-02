//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/align_ptr.h>
#include <base/logger.h>
#include <output_device/win32/wasapi.h>
#include <output_device/idevicetype.h>

namespace xamp::output_device::win32 {

class ExclusiveWasapiDeviceType final : public IDeviceType {
public:
	constexpr static auto Description = std::string_view("WASAPI (Exclusive Mode)");

	ExclusiveWasapiDeviceType() noexcept;

	void ScanNewDevice() override;

	[[nodiscard]] std::string_view GetDescription() const override;

	[[nodiscard]] Uuid GetTypeId() const override;

	[[nodiscard]] size_t GetDeviceCount() const override;

	[[nodiscard]] DeviceInfo GetDeviceInfo(uint32_t device) const override;

	[[nodiscard]] std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;

	[[nodiscard]] Vector<DeviceInfo> GetDeviceInfo() const override;

	AlignPtr<IOutputDevice> MakeDevice(std::string const & device_id) override;

private:
	void Initial();

	[[nodiscard]] CComPtr<IMMDevice> GetDeviceById(std::wstring const & device_id) const;

	[[nodiscard]] Vector<DeviceInfo> GetDeviceInfoList() const;

	CComPtr<IMMDeviceEnumerator> enumerator_;
	Vector<DeviceInfo> device_list_;
	std::shared_ptr<Logger> log_;
};
XAMP_MAKE_CLASS_UUID(ExclusiveWasapiDeviceType, "089F8446-C980-495B-AC80-5A437A4E73F6")

}

#endif

