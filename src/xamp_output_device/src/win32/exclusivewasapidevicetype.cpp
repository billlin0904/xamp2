#include <output_device/win32/hrexception.h>
#include <output_device/win32/exclusivewasapidevice.h>
#include <output_device/win32/exclusivewasapidevicetype.h>

namespace xamp::output_device::win32 {

const ID ExclusiveWasapiDeviceType::Id("089F8446-C980-495B-AC80-5A437A4E73F6");

ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceType() {
    HR_IF_FAILED_THROW(CoCreateInstance(__uuidof(MMDeviceEnumerator),
		nullptr, 
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<void**>(&enumerator_)));
    device_list_.push_back(GetDefaultOutputDeviceInfo());
}

DeviceInfo ExclusiveWasapiDeviceType::GetDefaultOutputDeviceInfo() const {
	CComPtr<IMMDevice> default_output_device;
	HR_IF_FAILED_THROW(enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &default_output_device));
	return helper::GetDeviceInfo(default_output_device, ExclusiveWasapiDeviceType::Id);
}

std::vector<DeviceInfo> ExclusiveWasapiDeviceType::GetDeviceInfo() const {
	return device_list_;
}

CComPtr<IMMDevice> ExclusiveWasapiDeviceType::GetDeviceById(const std::wstring& device_id) const {
	CComPtr<IMMDevice> device;
	HR_IF_FAILED_THROW(enumerator_->GetDevice(device_id.c_str(), &device));
	return device;
}

AlignPtr<Device> ExclusiveWasapiDeviceType::MakeDevice(const std::wstring& device_id) {
	return MakeAlign<Device, ExclusiveWasapiDevice>(GetDeviceById(device_id));
}

std::wstring ExclusiveWasapiDeviceType::GetName() const {
	return L"WASAPI (Exclusive)";
}

const ID& ExclusiveWasapiDeviceType::GetTypeId() const {
	return Id;
}

int32_t ExclusiveWasapiDeviceType::GetDeviceCount() const {
	return static_cast<int32_t>(device_list_.size());
}

DeviceInfo ExclusiveWasapiDeviceType::GetDeviceInfo(int32_t device) const {
	return device_list_.at(device);
}

void ExclusiveWasapiDeviceType::ScanNewDevice() {
	device_list_ = GetDeviceInfoList();
}

std::vector<DeviceInfo> ExclusiveWasapiDeviceType::GetDeviceInfoList() const {
	CComPtr<IMMDeviceCollection> devices;
	HR_IF_FAILED_THROW(enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices));

	UINT count = 0;
	HR_IF_FAILED_THROW(devices->GetCount(&count));

	std::vector<DeviceInfo> device_list;
	device_list.reserve(count);

	auto const default_device_info = GetDefaultOutputDeviceInfo();

	for (UINT i = 0; i < count; ++i) {
		CComPtr<IMMDevice> device;

		HR_IF_FAILED_THROW(devices->Item(i, &device));

		auto info = helper::GetDeviceInfo(device, ExclusiveWasapiDeviceType::Id);

		if (default_device_info.name == info.name) {
			info.is_default_device = true;
		}
		device_list.emplace_back(info);
	}

	std::sort(device_list.begin(), device_list.end(), [](const auto& first, const auto& second) {
		return first.name.length() > second.name.length();
		});

	return device_list;
}

}