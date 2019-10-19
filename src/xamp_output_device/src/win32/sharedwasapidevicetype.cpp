#ifdef _WIN32
#include <base/logger.h>
#include <output_device/win32/hrexception.h>
#include <output_device/win32/wasapi.h>
#include <output_device/win32/sharedwasapidevice.h>
#include <output_device/win32/sharedwasapidevicetype.h>

namespace xamp::output_device::win32 {

const ID SharedWasapiDeviceType::Id = ID("07885EDF-7CCB-4FA6-962D-B66A759978B1");

SharedWasapiDeviceType::SharedWasapiDeviceType() {
	HR_IF_FAILED_THROW(CoCreateInstance(__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<void**>(&enumerator_)));
}

void SharedWasapiDeviceType::ScanNewDevice() {
	device_list_ = GetDeviceInfoList();
}

CComPtr<IMMDevice> SharedWasapiDeviceType::GetDeviceById(const std::wstring& device_id) const {
	CComPtr<IMMDevice> device;
	HR_IF_FAILED_THROW(enumerator_->GetDevice(device_id.c_str(), &device));
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

std::wstring SharedWasapiDeviceType::GetName() const {
	return L"WASAPI (Shared)";
}

int32_t SharedWasapiDeviceType::GetDeviceCount() const {
	return int32_t(device_list_.size());
}

std::vector<DeviceInfo> SharedWasapiDeviceType::GetDeviceInfo() const {
	return device_list_;
}

DeviceInfo SharedWasapiDeviceType::GetDefaultDeviceInfo() const {
	CComPtr<IMMDevice> default_output_device;
	HR_IF_FAILED_THROW(enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &default_output_device));
	return helper::GetDeviceInfo(default_output_device, SharedWasapiDeviceType::Id);
}

std::vector<DeviceInfo> SharedWasapiDeviceType::GetDeviceInfoList() const {
	CComPtr<IMMDeviceCollection> devices;
	HR_IF_FAILED_THROW(enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices));

	UINT count = 0;
	HR_IF_FAILED_THROW(devices->GetCount(&count));

	std::vector<DeviceInfo> device_list;
	device_list.reserve(count);

	auto const default_device_info = GetDefaultDeviceInfo();

	for (UINT i = 0; i < count; ++i) {
		CComPtr<IMMDevice> device;

		HR_IF_FAILED_THROW(devices->Item(i, &device));

		try {
			auto info = helper::GetDeviceInfo(device, SharedWasapiDeviceType::Id);

			if (default_device_info.name == info.name) {
				info.is_default_device = true;
			}
			device_list.emplace_back(info);
		}
		catch (const Exception& ex) {
			XAMP_LOG_DEBUG(ex.what());
			throw;
		}
	}

	std::sort(device_list.begin(), device_list.end(),
		[](const auto& first, const auto& second) {
		return first.name.length() > second.name.length();
		});

	return std::move(device_list);
}

}
#endif

