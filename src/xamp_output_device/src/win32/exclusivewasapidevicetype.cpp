#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/scopeguard.h>
#include <base/str_utilts.h>
#include <base/logger_impl.h>
#include <output_device/win32/hrexception.h>
#include <output_device/win32/exclusivewasapidevice.h>
#include <output_device/win32/exclusivewasapidevicetype.h>

namespace xamp::output_device::win32 {

ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceType() noexcept {
	log_ = LoggerManager::GetInstance().GetLogger(kExclusiveWasapiDeviceTypeLoggerName);
	ScanNewDevice();
}

void ExclusiveWasapiDeviceType::Initial() {
	if (!enumerator_) {
		enumerator_ = helper::CreateDeviceEnumerator();
	}
}

void ExclusiveWasapiDeviceType::ScanNewDevice() {
	Initial();
	device_list_ = GetDeviceInfoList();
}

std::optional<DeviceInfo> ExclusiveWasapiDeviceType::GetDefaultDeviceInfo() const {
	CComPtr<IMMDevice> default_output_device;
	const auto hr = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &default_output_device);
	HrIfFailledThrow2(hr, ERROR_NOT_FOUND);
	if (hr == ERROR_NOT_FOUND) {
		return std::nullopt;
	}
	return helper::GetDeviceInfo(default_output_device, Id);
}

Vector<DeviceInfo> ExclusiveWasapiDeviceType::GetDeviceInfo() const {
	return device_list_;
}

CComPtr<IMMDevice> ExclusiveWasapiDeviceType::GetDeviceById(std::wstring const & device_id) const {
	CComPtr<IMMDevice> device;
	HrIfFailledThrow(enumerator_->GetDevice(device_id.c_str(), &device));
	return device;
}

AlignPtr<IOutputDevice> ExclusiveWasapiDeviceType::MakeDevice(std::string const & device_id) {
	return MakeAlign<IOutputDevice, ExclusiveWasapiDevice>(GetDeviceById(String::ToStdWString(device_id)));
}

std::string_view ExclusiveWasapiDeviceType::GetDescription() const {
	return Description;
}

Uuid ExclusiveWasapiDeviceType::GetTypeId() const {
	return Id;
}

size_t ExclusiveWasapiDeviceType::GetDeviceCount() const {
	return device_list_.size();
}

DeviceInfo ExclusiveWasapiDeviceType::GetDeviceInfo(uint32_t device) const {
	auto itr = device_list_.begin();
	if (device >= GetDeviceCount()) {
		throw DeviceNotFoundException();
	}
	std::advance(itr, device);
	return (*itr);
}

Vector<DeviceInfo> ExclusiveWasapiDeviceType::GetDeviceInfoList() const {
	CComPtr<IMMDeviceCollection> devices;
	HrIfFailledThrow(enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices));

	UINT count = 0;
	HrIfFailledThrow(devices->GetCount(&count));

	Vector<DeviceInfo> device_list;
	device_list.reserve(count);

	std::wstring default_device_name;
	if (const auto default_device_info = GetDefaultDeviceInfo()) {
		default_device_name = default_device_info.value().name;
	}

	XAMP_LOG_D(log_, "Load all devices");

	for (UINT i = 0; i < count; ++i) {
		CComPtr<IMMDevice> device;

		HrIfFailledThrow(devices->Item(i, &device));
		auto info = helper::GetDeviceInfo(device, Id);

		if (default_device_name == info.name) {
			info.is_default_device = true;
		}

		CComPtr<IAudioEndpointVolume> endpoint_volume;
		HrIfFailledThrow(device->Activate(__uuidof(IAudioEndpointVolume),
			CLSCTX_INPROC_SERVER,
			nullptr,
			reinterpret_cast<void**>(&endpoint_volume)
		));

		float min_volume = 0;
		float max_volume = 0;
		float volume_increment = 0;
		HrIfFailledThrow(endpoint_volume->GetVolumeRange(&min_volume, &max_volume, &volume_increment));
		info.min_volume = min_volume;
		info.max_volume = max_volume;
		info.volume_increment = volume_increment;

		XAMP_LOG_D(log_, "{} min_volume: {:.2f} dBFS, max_volume:{:.2f} dBFS, volume_increnment:{:.2f} dBFS, volume leve:{:.2f}.",
			info.device_id,
			info.min_volume,
			info.max_volume,
			info.volume_increment,
			(info.max_volume - info.min_volume) / info.volume_increment);

		DWORD volume_support_mask = 0;
		HrIfFailledThrow(endpoint_volume->QueryHardwareSupport(&volume_support_mask));

		info.is_hardware_control_volume = (volume_support_mask & ENDPOINT_HARDWARE_SUPPORT_VOLUME)
			&& (volume_support_mask & ENDPOINT_HARDWARE_SUPPORT_MUTE);

		// TODO: 一些DAC有支援WASAPI DOP模式.
		info.is_support_dsd = true;
		device_list.emplace_back(info);
	}

	std::sort(device_list.begin(), device_list.end(), 
		[](const auto& first, const auto& second) {
		return first.name.length() > second.name.length();
		});

	return device_list;
}

}
#endif
