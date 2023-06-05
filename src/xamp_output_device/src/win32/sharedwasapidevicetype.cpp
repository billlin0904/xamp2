#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/win32/wasapi.h>
#include <output_device/win32/comexception.h>
#include <output_device/win32/wasapi.h>
#include <output_device/win32/sharedwasapidevice.h>
#include <output_device/win32/sharedwasapidevicetype.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/str_utilts.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

class SharedWasapiDeviceType::SharedWasapiDeviceTypeImpl final {
public:
	SharedWasapiDeviceTypeImpl() noexcept;

	void ScanNewDevice();

	[[nodiscard]] size_t GetDeviceCount() const;

	[[nodiscard]] DeviceInfo GetDeviceInfo(uint32_t device) const;

	[[nodiscard]] std::optional<DeviceInfo> GetDefaultDeviceInfo() const;

	[[nodiscard]] Vector<DeviceInfo> GetDeviceInfo() const;

	AlignPtr<IOutputDevice> MakeDevice(const std::string& device_id);
	
private:
	[[nodiscard]] Vector<DeviceInfo> GetDeviceInfoList() const;

	[[nodiscard]] CComPtr<IMMDevice> GetDeviceById(const std::wstring& device_id) const;

	// Device enumerator
	CComPtr<IMMDeviceEnumerator> enumerator_;
	// Device list
	Vector<DeviceInfo> device_list_;
	// Logger
	LoggerPtr logger_;
};

SharedWasapiDeviceType::SharedWasapiDeviceTypeImpl::SharedWasapiDeviceTypeImpl() noexcept {
	logger_ = LoggerManager::GetInstance().GetLogger(kSharedWasapiDeviceLoggerName);
}

void SharedWasapiDeviceType::SharedWasapiDeviceTypeImpl::ScanNewDevice() {
	enumerator_ = helper::CreateDeviceEnumerator();
	device_list_ = GetDeviceInfoList();
}

CComPtr<IMMDevice> SharedWasapiDeviceType::SharedWasapiDeviceTypeImpl::GetDeviceById(std::wstring const & device_id) const {
	CComPtr<IMMDevice> device;
	HrIfFailledThrow(enumerator_->GetDevice(device_id.c_str(), &device));
	return device;
}

AlignPtr<IOutputDevice> SharedWasapiDeviceType::SharedWasapiDeviceTypeImpl::MakeDevice(std::string const & device_id) {
	return MakeAlign<IOutputDevice, SharedWasapiDevice>(GetDeviceById(String::ToStdWString(device_id)));
}

DeviceInfo SharedWasapiDeviceType::SharedWasapiDeviceTypeImpl::GetDeviceInfo(uint32_t device) const {
	auto itr = device_list_.begin();
	if (device >= GetDeviceCount()) {
		throw DeviceNotFoundException();
	}
	std::advance(itr, device);
	return (*itr);
}

size_t SharedWasapiDeviceType::SharedWasapiDeviceTypeImpl::GetDeviceCount() const {
	return device_list_.size();
}

Vector<DeviceInfo> SharedWasapiDeviceType::SharedWasapiDeviceTypeImpl::GetDeviceInfo() const {
	return device_list_;
}

std::optional<DeviceInfo> SharedWasapiDeviceType::SharedWasapiDeviceTypeImpl::GetDefaultDeviceInfo() const {
	CComPtr<IMMDevice> default_output_device;
	auto hr = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &default_output_device);
	HrIfFailledThrow2(hr, ERROR_NOT_FOUND);
	if (hr == ERROR_NOT_FOUND) {
		return std::nullopt;
	}
	return helper::GetDeviceInfo(default_output_device, XAMP_UUID_OF(SharedWasapiDeviceType));
}

Vector<DeviceInfo> SharedWasapiDeviceType::SharedWasapiDeviceTypeImpl::GetDeviceInfoList() const {
	CComPtr<IMMDeviceCollection> devices;
	UINT count = 0;
	Vector<DeviceInfo> device_list;
	std::wstring default_device_name;

	try {
		// Get all active devices
		HrIfFailledThrow(enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices));

		// Get device count
		HrIfFailledThrow(devices->GetCount(&count));

		device_list.reserve(count);

		if (const auto default_device_info = GetDefaultDeviceInfo()) {
			default_device_name = default_device_info.value().name;
		}
	}
	catch (const std::exception& e) {
		XAMP_LOG_E(logger_, "Fail to list active device! {}", e.what());
		return device_list;
	}	

	XAMP_LOG_D(logger_, "Load all devices");

	for (UINT i = 0; i < count; ++i) {
		CComPtr<IMMDevice> device;

		try {
			HrIfFailledThrow(devices->Item(i, &device));

			auto info = helper::GetDeviceInfo(device, XAMP_UUID_OF(SharedWasapiDeviceType));
			if (default_device_name == info.name) {
				info.is_default_device = true;
			}

			// Shared mode device always support hardware volume control
			info.is_hardware_control_volume = true;
			// Shared mode device always not support DSD
			info.is_support_dsd = false;
			device_list.push_back(info);
		}
		catch (const std::exception& e) {
			XAMP_LOG_D(logger_, "Load device failed: {}", e.what());
		}
	}

	std::sort(device_list.begin(), device_list.end(),
		[](const auto& first, const auto& second) {
		return first.name.length() > second.name.length();
		});

	return device_list;
}

SharedWasapiDeviceType::SharedWasapiDeviceType() noexcept
	: impl_(MakePimpl<SharedWasapiDeviceTypeImpl>()) {
}

void SharedWasapiDeviceType::ScanNewDevice() {
	impl_->ScanNewDevice();
}

std::string_view SharedWasapiDeviceType::GetDescription() const {
	return Description;
}

Uuid SharedWasapiDeviceType::GetTypeId() const {
	return XAMP_UUID_OF(SharedWasapiDeviceType);
}

size_t SharedWasapiDeviceType::GetDeviceCount() const {
	return impl_->GetDeviceCount();
}

DeviceInfo SharedWasapiDeviceType::GetDeviceInfo(uint32_t device) const {
	return impl_->GetDeviceInfo(device);
}

std::optional<DeviceInfo> SharedWasapiDeviceType::GetDefaultDeviceInfo() const {
	return impl_->GetDefaultDeviceInfo();
}

Vector<DeviceInfo> SharedWasapiDeviceType::GetDeviceInfo() const {
	return impl_->GetDeviceInfo();
}

AlignPtr<IOutputDevice> SharedWasapiDeviceType::MakeDevice(const std::string& device_id) {
	return impl_->MakeDevice(device_id);
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif

