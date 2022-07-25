#if ENABLE_ASIO
#include <asiodrivers.h>

#include <base/align_ptr.h>
#include <base/str_utilts.h>

#include <output_device/win32/asiodevice.h>
#include <output_device/win32/asiodevicetype.h>

namespace xamp::output_device::win32 {

HashMap<std::string, DeviceInfo> ASIODeviceType::device_info_cache_;

ASIODeviceType::ASIODeviceType() = default;

std::string_view ASIODeviceType::GetDescription() const {
	return Description;
}

Uuid ASIODeviceType::GetTypeId() const {
	return Id;
}

size_t ASIODeviceType::GetDeviceCount() const {
	return device_info_cache_.size();
}

DeviceInfo ASIODeviceType::GetDeviceInfo(uint32_t device) const {
	auto itr = device_info_cache_.begin();
	if (device >= GetDeviceCount()) {
		throw DeviceNotFoundException();
	}
	std::advance(itr, device);
	return (*itr).second;
}

std::optional<DeviceInfo> ASIODeviceType::GetDefaultDeviceInfo() const {
	if (device_info_cache_.empty()) {
		return std::nullopt;
	}
	return GetDeviceInfo(0);
}

Vector<DeviceInfo> ASIODeviceType::GetDeviceInfo() const {
	Vector<DeviceInfo> device_infos;
	device_infos.reserve(device_info_cache_.size());

	for (const auto& device_info : device_info_cache_) {
		device_infos.push_back(device_info.second);
	}
	return device_infos;
}

void ASIODeviceType::ScanNewDevice() {
    constexpr auto kMaxPathLen = 256;

	AsioDrivers drivers;
	const auto num_device = drivers.asioGetNumDev();

	for (auto i = 0; i < num_device; ++i) {		
		CLSID clsid{ 0 };
		if (drivers.asioGetDriverCLSID(i, &clsid) == 0) {
			char driver_name[kMaxPathLen + 1]{};
			drivers.asioGetDriverName(i, driver_name, kMaxPathLen);
			if (!device_info_cache_.contains(driver_name)) {
				device_info_cache_[driver_name] = GetDeviceInfo(String::ToStdWString(driver_name), driver_name);
			}			
		}
	}
}

DeviceInfo ASIODeviceType::GetDeviceInfo(std::wstring const& name, std::string const & device_id) const {
	DeviceInfo info;
	info.name = name;
	info.device_id = device_id;
	info.device_type_id = Id;
	/*try {
		const auto device = MakeAlign<IOutputDevice, AsioDevice>(device_id);
		auto* asio_device = dynamic_cast<AsioDevice*>(device.get());
		asio_device->OpenStream(AudioFormat::kPCM441Khz);
		info.is_support_dsd = asio_device->IsSupportDsdFormat();
		info.is_hardware_control_volume = asio_device->IsHardwareControlVolume();
		info.is_support_dsd = true;
		info.is_hardware_control_volume = true;
	} catch (Exception const &e) {
		XAMP_LOG_DEBUG(e.what());
	}*/
	info.is_support_dsd = true;
	info.is_hardware_control_volume = true;
	return info;
}

AlignPtr<IOutputDevice> ASIODeviceType::MakeDevice(std::string const & device_id) {
	return MakeAlign<IOutputDevice, AsioDevice>(device_id);
}

}
#endif
