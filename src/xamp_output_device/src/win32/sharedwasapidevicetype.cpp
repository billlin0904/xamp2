#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/logger.h>
#include <base/str_utilts.h>
#include <output_device/win32/hrexception.h>
#include <output_device/win32/wasapi.h>
#include <output_device/win32/sharedwasapidevice.h>
#include <output_device/win32/sharedwasapidevicetype.h>

namespace xamp::output_device::win32 {

SharedWasapiDeviceType::SharedWasapiDeviceType() noexcept {
	ScanNewDevice();
}

void SharedWasapiDeviceType::ScanNewDevice() {
	Initial();
	device_list_ = GetDeviceInfoList();
}

void SharedWasapiDeviceType::Initial() {
	if (!enumerator_) {
		enumerator_ = helper::CreateDeviceEnumerator();
	}
}

CComPtr<IMMDevice> SharedWasapiDeviceType::GetDeviceById(std::wstring const & device_id) const {
	CComPtr<IMMDevice> device;
	HrIfFailledThrow(enumerator_->GetDevice(device_id.c_str(), &device));
	return device;
}

AlignPtr<Device> SharedWasapiDeviceType::MakeDevice(std::string const & device_id) {
	return MakeAlign<Device, SharedWasapiDevice>(GetDeviceById(String::ToStdWString(device_id)));
}

DeviceInfo SharedWasapiDeviceType::GetDeviceInfo(uint32_t device) const {
	auto itr = device_list_.begin();
	if (device >= GetDeviceCount()) {
		throw DeviceNotFoundException();
	}
	std::advance(itr, device);
	return (*itr);
}

Uuid SharedWasapiDeviceType::GetTypeId() const {
	return Id;
}

std::string_view SharedWasapiDeviceType::GetDescription() const {
	return Description;
}

size_t SharedWasapiDeviceType::GetDeviceCount() const {
	return device_list_.size();
}

std::vector<DeviceInfo> SharedWasapiDeviceType::GetDeviceInfo() const {
	return device_list_;
}

std::optional<DeviceInfo> SharedWasapiDeviceType::GetDefaultDeviceInfo() const {
	CComPtr<IMMDevice> default_output_device;
	auto hr = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &default_output_device);
	HrIfFailledThrow2(hr, ERROR_NOT_FOUND);
	if (hr == ERROR_NOT_FOUND) {
		return std::nullopt;
	}
	return helper::GetDeviceInfo(default_output_device, Id);
}

std::vector<DeviceInfo> SharedWasapiDeviceType::GetDeviceInfoList() const {
	CComPtr<IMMDeviceCollection> devices;
	HrIfFailledThrow(enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices));

	UINT count = 0;
	HrIfFailledThrow(devices->GetCount(&count));

	std::vector<DeviceInfo> device_list;
	device_list.reserve(count);

	std::wstring default_device_name;
	if (const auto default_device_info = GetDefaultDeviceInfo()) {
		default_device_name = default_device_info.value().name;
	}

	for (UINT i = 0; i < count; ++i) {
		CComPtr<IMMDevice> device;

		HrIfFailledThrow(devices->Item(i, &device));

		auto info = helper::GetDeviceInfo(device, Id);
#ifdef _DEBUG
		XAMP_LOG_TRACE("Get {} device {} property.", GetDescription(), String::ToUtf8String(info.name));
		for (const auto& property : helper::GetDeviceProperty(device)) {
			XAMP_LOG_TRACE("{}: {}", property.first, String::ToUtf8String(property.second));
		}
#endif  
		if (default_device_name == info.name) {
			info.is_default_device = true;
		}
		info.is_support_dsd = false;
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

