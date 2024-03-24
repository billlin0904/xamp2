#include <output_device/win32/exclusivewasapidevicetype.h>

#ifdef XAMP_OS_WIN
#include <output_device/win32/comexception.h>
#include <output_device/win32/exclusivewasapidevice.h>
#include <output_device/win32/wasapi.h>

#include <base/base.h>
#include <base/scopeguard.h>
#include <base/str_utilts.h>
#include <base/logger_impl.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(ExclusiveWasapiDeviceType);

class ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl final {
public:
	ExclusiveWasapiDeviceTypeImpl() noexcept;

	~ExclusiveWasapiDeviceTypeImpl() = default;

	void ScanNewDevice();

	[[nodiscard]] size_t GetDeviceCount() const;

	[[nodiscard]] DeviceInfo GetDeviceInfo(uint32_t device) const;

	[[nodiscard]] std::optional<DeviceInfo> GetDefaultDeviceInfo() const;

	[[nodiscard]] Vector<DeviceInfo> GetDeviceInfo() const;

	AlignPtr<IOutputDevice> MakeDevice(const std::string& device_id);

private:
	[[nodiscard]] CComPtr<IMMDevice> GetDeviceById(const std::wstring& device_id) const;

	[[nodiscard]] Vector<DeviceInfo> GetDeviceInfoList() const;

	// Device enumerator
	CComPtr<IMMDeviceEnumerator> enumerator_;
	// Device list
	Vector<DeviceInfo> device_list_;
	// Logger
	LoggerPtr logger_;
};

ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::ExclusiveWasapiDeviceTypeImpl() noexcept {
	logger_ = XampLoggerFactory.GetLogger(kExclusiveWasapiDeviceTypeLoggerName);	
}

void ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::ScanNewDevice() {
	enumerator_ = helper::CreateDeviceEnumerator();
	device_list_ = GetDeviceInfoList();	
}

std::optional<DeviceInfo> ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::GetDefaultDeviceInfo() const {
	CComPtr<IMMDevice> default_output_device;
	const auto hr = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &default_output_device);
	HrIfFailledThrow2(hr, ERROR_NOT_FOUND);
	if (hr == ERROR_NOT_FOUND) {
		return std::nullopt;
	}
	return helper::GetDeviceInfo(default_output_device, XAMP_UUID_OF(ExclusiveWasapiDeviceType));
}

Vector<DeviceInfo> ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::GetDeviceInfo() const {
	return device_list_;
}

CComPtr<IMMDevice> ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::GetDeviceById(const std::wstring & device_id) const {
	CComPtr<IMMDevice> device;
	HrIfFailledThrow(enumerator_->GetDevice(device_id.c_str(), &device));
	return device;
}

AlignPtr<IOutputDevice> ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::MakeDevice(const std::string & device_id) {
	return MakeAlign<IOutputDevice, ExclusiveWasapiDevice>(GetDeviceById(String::ToStdWString(device_id)));
}

size_t ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::GetDeviceCount() const {
	return device_list_.size();
}

DeviceInfo ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::GetDeviceInfo(uint32_t device) const {
	auto itr = device_list_.begin();
	if (device >= GetDeviceCount()) {
		throw DeviceNotFoundException();
	}
	std::advance(itr, device);
	return (*itr);
}

Vector<DeviceInfo> ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::GetDeviceInfoList() const {
	CComPtr<IMMDeviceCollection> devices;
	Vector<DeviceInfo> device_list;
	std::wstring default_device_name;
	UINT count = 0;

	try {
		// Get all active devices
		HrIfFailledThrow(enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices));

		// Get device count
		HrIfFailledThrow(devices->GetCount(&count));

		device_list.reserve(count);

		if (const auto default_device_info = GetDefaultDeviceInfo()) {
			default_device_name = default_device_info.value().name;
		}
	} catch (const std::exception& e) {
		XAMP_LOG_E(logger_, "Fail to list active device! {}", e.what());
		return device_list;
	}	

	XAMP_LOG_D(logger_, "Load all devices");

	for (UINT i = 0; i < count; ++i) {
		CComPtr<IMMDevice> device;

		try {
			HrIfFailledThrow(devices->Item(i, &device));

			auto info = helper::GetDeviceInfo(device, XAMP_UUID_OF(ExclusiveWasapiDeviceType));

			// Check device support exclusive mode
			CComPtr<IAudioClient> client;
			auto hr = device->Activate(__uuidof(IAudioClient),
				CLSCTX_ALL,
				nullptr,
				reinterpret_cast<void**>(&client));
			if (SUCCEEDED(hr)) {
				WAVEFORMATEX* format = nullptr;
				hr = client->GetMixFormat(&format);
				if (FAILED(hr)) {
					continue;
				}
				CComHeapPtr<WAVEFORMATEX> mix_format(format);
				info.default_format = helper::ToAudioFormat(format);
			}

			if (default_device_name == info.name) {
				info.is_default_device = true;
			}

			if (info.default_format) {
				XAMP_LOG_D(logger_, "{} default format: {}", String::ToString(info.name), info.default_format.value());
			}

			CComPtr<IAudioEndpointVolume> endpoint_volume;
			HrIfFailledThrow(device->Activate(__uuidof(IAudioEndpointVolume),
				CLSCTX_INPROC_SERVER,
				nullptr,
				reinterpret_cast<void**>(&endpoint_volume)
			));

			DWORD volume_support_mask = 0;
			HrIfFailledThrow(endpoint_volume->QueryHardwareSupport(&volume_support_mask));

			// Check device support volume control
			info.is_hardware_control_volume = (volume_support_mask & ENDPOINT_HARDWARE_SUPPORT_VOLUME)
				&& (volume_support_mask & ENDPOINT_HARDWARE_SUPPORT_MUTE);

			// Exclusive mode device always support DSD
			info.is_support_dsd = true;

			device_list.push_back(info);
		} catch (const std::exception& e) {
			XAMP_LOG_D(logger_, "Load device failed: {}", e.what());
		}
	}

	// Sort device list by name length
	std::sort(device_list.begin(), device_list.end(),
	                  [](const auto& first, const auto& second) {
		                  return first.name.length() > second.name.length();
	                  });

	return device_list;
}

XAMP_PIMPL_IMPL(ExclusiveWasapiDeviceType)

ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceType() noexcept
	: impl_(MakeAlign<ExclusiveWasapiDeviceTypeImpl>()) {
}

void ExclusiveWasapiDeviceType::ScanNewDevice() {
	impl_->ScanNewDevice();
}

std::string_view ExclusiveWasapiDeviceType::GetDescription() const {
	return Description;
}

Uuid ExclusiveWasapiDeviceType::GetTypeId() const {
	return XAMP_UUID_OF(ExclusiveWasapiDeviceType);
}

size_t ExclusiveWasapiDeviceType::GetDeviceCount() const {
	return impl_->GetDeviceCount();
}

DeviceInfo ExclusiveWasapiDeviceType::GetDeviceInfo(uint32_t device) const {
	return impl_->GetDeviceInfo(device);
}

std::optional<DeviceInfo> ExclusiveWasapiDeviceType::GetDefaultDeviceInfo() const {
	return impl_->GetDefaultDeviceInfo();
}

Vector<DeviceInfo> ExclusiveWasapiDeviceType::GetDeviceInfo() const {
	return impl_->GetDeviceInfo();
}

AlignPtr<IOutputDevice> ExclusiveWasapiDeviceType::MakeDevice(std::string const& device_id) {
	return impl_->MakeDevice(device_id);
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif
