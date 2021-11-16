#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/str_utilts.h>
#include <output_device/win32/hrexception.h>
#include <output_device/win32/exclusivewasapidevice.h>
#include <output_device/win32/exclusivewasapidevicetype.h>

namespace xamp::output_device::win32 {

ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceType() noexcept {
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

std::vector<DeviceInfo> ExclusiveWasapiDeviceType::GetDeviceInfo() const {
	return device_list_;
}

CComPtr<IMMDevice> ExclusiveWasapiDeviceType::GetDeviceById(std::wstring const & device_id) const {
	CComPtr<IMMDevice> device;
	HrIfFailledThrow(enumerator_->GetDevice(device_id.c_str(), &device));
	return device;
}

AlignPtr<IDevice> ExclusiveWasapiDeviceType::MakeDevice(std::string const & device_id) {
	return MakeAlign<IDevice, ExclusiveWasapiDevice>(GetDeviceById(String::ToStdWString(device_id)));
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

std::vector<DeviceInfo> ExclusiveWasapiDeviceType::GetDeviceInfoList() const {
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

		// 不是所有設備都支援獨佔模式, 可以透過IsFormatSupported進行檢查.
		CComPtr<IAudioClient2> client;
		auto hr = device->Activate(__uuidof(IAudioClient2),
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&client));

		if (SUCCEEDED(hr)) {
			CComHeapPtr<WAVEFORMATEX> mix_format;
			hr = client->GetMixFormat(&mix_format);
			if (hr != S_OK) {
				XAMP_LOG_ERROR(HRException::ErrorToStringHelper((hr)));
				continue;
			}
			hr = client->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, mix_format, nullptr);
			if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
				continue;
			} else if (hr != S_OK) {
				XAMP_LOG_ERROR(HRException::ErrorToStringHelper((hr)));
				continue;
			}
		} else {
			XAMP_LOG_ERROR(HRException::ErrorToStringHelper((hr)));
		}

		if (default_device_name == info.name) {
			info.is_default_device = true;
		}

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
