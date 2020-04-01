#ifdef _WIN32
#include <base/logger.h>
#include <output_device/win32/hrexception.h>
#include <output_device/win32/wasapi.h>
#include <output_device/win32/sharedwasapidevice.h>
#include <output_device/win32/sharedwasapidevicetype.h>

namespace xamp::output_device::win32 {

const ID SharedWasapiDeviceType::Id = ID("07885EDF-7CCB-4FA6-962D-B66A759978B1");

SharedWasapiDeviceType::SharedWasapiDeviceType() {
}

void SharedWasapiDeviceType::ScanNewDevice() {
	Initial();
	device_list_ = GetDeviceInfoList();
}

void SharedWasapiDeviceType::Initial() {
	if (!enumerator_) {
		HrIfFailledThrow(CoCreateInstance(__uuidof(MMDeviceEnumerator),
			nullptr,
			CLSCTX_ALL,
			__uuidof(IMMDeviceEnumerator),
			reinterpret_cast<void**>(&enumerator_)));
	}
}

CComPtr<IMMDevice> SharedWasapiDeviceType::GetDeviceById(const std::wstring& device_id) const {
	CComPtr<IMMDevice> device;
	HrIfFailledThrow(enumerator_->GetDevice(device_id.c_str(), &device));
	return device;
}

AlignPtr<Device> SharedWasapiDeviceType::MakeDevice(const std::wstring& device_id) {
	return MakeAlign<Device, SharedWasapiDevice>(GetDeviceById(device_id));
}

DeviceInfo SharedWasapiDeviceType::GetDeviceInfo(int32_t device) const {
	return device_list_[device];
}

const ID& SharedWasapiDeviceType::GetTypeId() const {
	return Id;
}

std::string_view SharedWasapiDeviceType::GetDescription() const {
	return "WASAPI (Shared)";
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
	return helper::GetDeviceInfo(default_output_device, SharedWasapiDeviceType::Id);
}

std::vector<DeviceInfo> SharedWasapiDeviceType::GetDeviceInfoList() const {
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

		auto info = helper::GetDeviceInfo(device, SharedWasapiDeviceType::Id);

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

	return std::move(device_list);
}

}
#endif

