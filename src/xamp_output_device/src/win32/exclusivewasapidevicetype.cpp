#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <output_device/win32/hrexception.h>
#include <output_device/win32/exclusivewasapidevice.h>
#include <output_device/win32/exclusivewasapidevicetype.h>

namespace xamp::output_device::win32 {

const ID ExclusiveWasapiDeviceType::Id("089F8446-C980-495B-AC80-5A437A4E73F6");

ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceType() {    
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
	auto hr = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &default_output_device);
	HrIfFailledThrow2(hr, ERROR_NOT_FOUND);
	if (hr == ERROR_NOT_FOUND) {
		return std::nullopt;
	}	
	return helper::GetDeviceInfo(default_output_device, ExclusiveWasapiDeviceType::Id);
}

std::vector<DeviceInfo> ExclusiveWasapiDeviceType::GetDeviceInfo() const {
	return device_list_;
}

CComPtr<IMMDevice> ExclusiveWasapiDeviceType::GetDeviceById(const std::wstring& device_id) const {
	CComPtr<IMMDevice> device;
	HrIfFailledThrow(enumerator_->GetDevice(device_id.c_str(), &device));
	return device;
}

align_ptr<Device> ExclusiveWasapiDeviceType::MakeDevice(const std::wstring& device_id) {
	return MakeAlign<Device, ExclusiveWasapiDevice>(GetDeviceById(device_id));
}

std::string_view ExclusiveWasapiDeviceType::GetDescription() const {
	return "WASAPI (Exclusive)";
}

const ID& ExclusiveWasapiDeviceType::GetTypeId() const {
	return Id;
}

size_t ExclusiveWasapiDeviceType::GetDeviceCount() const {
	return device_list_.size();
}

DeviceInfo ExclusiveWasapiDeviceType::GetDeviceInfo(uint32_t device) const {
	auto itr = device_list_.begin();
	std::advance(itr, device);
	if (itr != device_list_.end()) {
		return (*itr);
	}
	throw Exception(Errors::XAMP_ERROR_DEVICE_NOT_FOUND);
}

std::vector<DeviceInfo> ExclusiveWasapiDeviceType::GetDeviceInfoList() const {
	CComPtr<IMMDeviceCollection> devices;
	HrIfFailledThrow(enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices));

	UINT count = 0;
	HrIfFailledThrow(devices->GetCount(&count));

	std::vector<DeviceInfo> device_list;
	device_list.reserve(count);

	const auto default_device_info = GetDefaultDeviceInfo();

	for (UINT i = 0; i < count; ++i) {
		CComPtr<IMMDevice> device;

		HrIfFailledThrow(devices->Item(i, &device));

		auto info = helper::GetDeviceInfo(device, ExclusiveWasapiDeviceType::Id);

		if (default_device_info) {
			if (default_device_info.value().name == info.name) {
				info.is_default_device = true;
			}
		}		
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
