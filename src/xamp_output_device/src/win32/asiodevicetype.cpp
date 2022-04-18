#if ENABLE_ASIO
#include <asiodrivers.h>

#include <base/align_ptr.h>
#include <base/str_utilts.h>

#include <output_device/win32/asiodevice.h>
#include <output_device/win32/asiodevicetype.h>

namespace xamp::output_device::win32 {

ASIODeviceType::ASIODeviceType() = default;

std::string_view ASIODeviceType::GetDescription() const {
	return Description;
}

Uuid ASIODeviceType::GetTypeId() const {
	return Id;
}

size_t ASIODeviceType::GetDeviceCount() const {
	return device_list_.size();
}

DeviceInfo ASIODeviceType::GetDeviceInfo(uint32_t device) const {
	auto itr = device_list_.begin();
	if (device >= GetDeviceCount()) {
		throw DeviceNotFoundException();
	}
	std::advance(itr, device);
	return (*itr).second;
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
    constexpr auto kMaxPathLen = 256;

	device_list_.clear();

	AsioDrivers drivers;
	const auto num_device = drivers.asioGetNumDev();

	for (auto i = 0; i < num_device; ++i) {		
		CLSID clsid{ 0 };
		if (drivers.asioGetDriverCLSID(i, &clsid) == 0) {
			char driver_name[kMaxPathLen + 1]{};
			drivers.asioGetDriverName(i, driver_name, kMaxPathLen);
			if (device_list_.find(driver_name) != device_list_.end()) {
				continue;
			}
			device_list_[driver_name] = GetDeviceInfo(String::ToStdWString(driver_name), driver_name);
		}
	}
}

DeviceInfo ASIODeviceType::GetDeviceInfo(std::wstring const& name, std::string const & device_id) const {
	DeviceInfo info;
	info.name = name;
	info.device_id = device_id;
	info.device_type_id = Id;
	// NOTE: 無法不開啟ASIO狀態下取得是否支援DSD, 目前先假定ASIO是支援DSD的!
	info.is_support_dsd = true;
	return info;
}

AlignPtr<IOutputDevice> ASIODeviceType::MakeDevice(std::string const & device_id) {
	return MakeAlign<IOutputDevice, AsioDevice>(device_id);
}

}
#endif
