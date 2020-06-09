// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#if ENABLE_ASIO
#include <asiosys.h>
#include <asio.h>
#include <asiodrivers.h>

#include <base/str_utilts.h>

#include <output_device/asioexception.h>
#include <output_device/asiodevice.h>
#include <output_device/asiodevicetype.h>

#ifdef XAMP_OS_WIN
#include <iasiodrv.h>
#endif

namespace xamp::output_device {

std::string_view const ASIODeviceType::Id("0B3FF8BC-5BFD-4A08-8066-95974FB11BB5");

ASIODeviceType::ASIODeviceType() {
}

std::string_view ASIODeviceType::GetDescription() const {
	return "ASIO";
}

ID ASIODeviceType::GetTypeId() const {
	return Id;
}

size_t ASIODeviceType::GetDeviceCount() const {
	return device_list_.size();
}

DeviceInfo ASIODeviceType::GetDeviceInfo(uint32_t device) const {
	auto itr = device_list_.begin();
	std::advance(itr, device);
	if (itr != device_list_.end()) {
		return (*itr).second;
	}
	throw ASIOException(Errors::XAMP_ERROR_DEVICE_NOT_FOUND);
}

std::optional<DeviceInfo> ASIODeviceType::GetDefaultDeviceInfo() const {
	if (device_list_.empty()) {
		return std::nullopt;
	}
	return GetDeviceInfo(0);
}

std::vector<DeviceInfo> ASIODeviceType::GetDeviceInfo() const {
	std::vector<DeviceInfo> device_infos;
	device_infos.reserve(device_list_.size());

	for (const auto& device_info : device_list_) {
		device_infos.push_back(device_info.second);
	}
	return device_infos;
}

void ASIODeviceType::ScanNewDevice() {
    constexpr auto MAX_PATH_LEN = 256;

	device_list_.clear();

	AsioDrivers drivers;
	const auto num_device = drivers.asioGetNumDev();

	for (auto i = 0; i < num_device; ++i) {
        char driver_name[MAX_PATH_LEN]{};
        if (drivers.asioGetDriverName(i, driver_name, MAX_PATH_LEN) == 0) {
			const auto name = ToStdWString(driver_name);
			if (device_list_.find(name) != device_list_.end()) {
				continue;
			}
			device_list_[name] = GetDeviceInfo(name);
		}
	}
}

DeviceInfo ASIODeviceType::GetDeviceInfo(std::wstring const & device_id) const {
	DeviceInfo info;
	info.name = device_id;
	info.device_id = device_id;
	info.device_type_id = Id;
	// NOTE: 無法不開啟ASIO狀態下取得是否支援DSD, 目前先假定ASIO是支援DSD的!
	info.is_support_dsd = true;
	return info;
}

AlignPtr<Device> ASIODeviceType::MakeDevice(std::wstring const & device_id) {
	const auto id = ToUtf8String(device_id);
	return MakeAlign<Device, AsioDevice>(id);
}

}
#endif
