#include <output_device/win32/exclusivewasapidevicetype.h>

#ifdef XAMP_OS_WIN
#include <output_device/win32/comexception.h>
#include <output_device/win32/exclusivewasapidevice.h>
#include <output_device/win32/wasapi.h>

#include <base/base.h>
#include <base/scopeguard.h>
#include <base/str_utilts.h>
#include <base/logger.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(ExclusiveWasapiDeviceType);

class ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl final {
public:
	ExclusiveWasapiDeviceTypeImpl() ;

	~ExclusiveWasapiDeviceTypeImpl() = default;

	void scanNewDevice();

	[[nodiscard]] size_t getDeviceCount() const;

	[[nodiscard]] DeviceInfo getDeviceInfo(uint32_t device) const;

	[[nodiscard]] std::optional<DeviceInfo> getDefaultDeviceInfo() const;

	[[nodiscard]] std::vector<DeviceInfo> getDeviceInfo() const;

	ScopedPtr<IOutputDevice> makeDevice(const std::shared_ptr<IThreadPool>& thread_pool, const std::string& device_id);

private:
	[[nodiscard]] CComPtr<IMMDevice> getDeviceById(const std::wstring& device_id) const;

	[[nodiscard]] std::vector<DeviceInfo> getDeviceInfoList() const;

	// Device enumerator
	CComPtr<IMMDeviceEnumerator> enumerator_;
	// Device list
	std::vector<DeviceInfo> device_list_;
	// Logger
	LoggerPtr logger_;
};

ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::ExclusiveWasapiDeviceTypeImpl() {
	logger_ = XampLoggerFactory.getLogger(XAMP_LOG_NAME(ExclusiveWasapiDeviceType));	
}

void ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::scanNewDevice() {
	enumerator_ = helper::createDeviceEnumerator();
	device_list_ = getDeviceInfoList();	
}

std::optional<DeviceInfo> ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::getDefaultDeviceInfo() const {
	CComPtr<IMMDevice> default_output_device;
	const auto hr = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &default_output_device);
	constexpr auto kNotFoundHr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	hIfNotEqualThrow(hr, kNotFoundHr);
	if (hr == kNotFoundHr) {
		return std::nullopt;
	}
	return makeOptional<DeviceInfo>(helper::getDeviceInfo(default_output_device,
		XAMP_UUID_OF(ExclusiveWasapiDeviceType), 
		ExclusiveWasapiDeviceType::Description));
}

std::vector<DeviceInfo> ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::getDeviceInfo() const {
	return device_list_;
}

CComPtr<IMMDevice> ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::getDeviceById(const std::wstring & device_id) const {
	CComPtr<IMMDevice> device;
	hrIfFailThrow(enumerator_->GetDevice(device_id.c_str(), &device));
	return device;
}

ScopedPtr<IOutputDevice> ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::makeDevice(const std::shared_ptr<IThreadPool>& /*thread_pool*/, const std::string & device_id) {
	return makeAlign<IOutputDevice, ExclusiveWasapiDevice>(getDeviceById(String::toStdWString(device_id)));
}

size_t ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::getDeviceCount() const {
	return device_list_.size();
}

DeviceInfo ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::getDeviceInfo(uint32_t device) const {
	auto itr = device_list_.begin();
	if (device >= getDeviceCount()) {
		throw DeviceNotFoundException();
	}
	std::advance(itr, device);
	return (*itr);
}

std::vector<DeviceInfo> ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceTypeImpl::getDeviceInfoList() const {
	CComPtr<IMMDeviceCollection> devices;
	std::vector<DeviceInfo> device_list;
	std::wstring default_device_name;
	UINT count = 0;

	try {
		// Get all active devices
		hrIfFailThrow(enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices));

		// Get device count
		hrIfFailThrow(devices->GetCount(&count));

		device_list.reserve(count);

		if (const auto default_device_info = getDefaultDeviceInfo()) {
			default_device_name = default_device_info.value().name;
		}
	} catch (const std::exception& e) {
		XAMP_LOG_E(logger_, "Fail to list active device! {}", e.what());
		return device_list;
	}	

	XAMP_LOG_D(logger_, "load all devices");

	for (UINT i = 0; i < count; ++i) {
		CComPtr<IMMDevice> device;

		try {
			hrIfFailThrow(devices->Item(i, &device));

			auto info = helper::getDeviceInfo(device, XAMP_UUID_OF(ExclusiveWasapiDeviceType), ExclusiveWasapiDeviceType::Description);

			AudioFormat default_format;
			if (!helper::isDeviceSupportExclusiveMode(device, default_format)) {
				continue;
			}			

			if (default_device_name == info.name) {
				info.is_default_device = true;
			}

			info.default_format = default_format;
			if (info.default_format) {
				XAMP_LOG_D(logger_, "{} default format: {}", String::toString(info.name), info.default_format.value());
			}

			CComPtr<IAudioEndpointVolume> endpoint_volume;
			hrIfFailThrow(device->Activate(__uuidof(IAudioEndpointVolume),
				CLSCTX_INPROC_SERVER,
				nullptr,
				reinterpret_cast<void**>(&endpoint_volume)
			));

			float scaled_min_db = 0;
			float scaled_max_db = 0;
			float volume_increment = 0;
			hrIfFailThrow(endpoint_volume->GetVolumeRange(&scaled_min_db,
				&scaled_max_db,
				&volume_increment));
			info.scaled_min_db = scaled_min_db;
			info.scaled_max_db = scaled_max_db;
			info.volume_increment  = volume_increment;
			info.is_normalized_volume = true;

			DWORD volume_support_mask = 0;
			hrIfFailThrow(endpoint_volume->QueryHardwareSupport(&volume_support_mask));

			// Check device support volume control
			info.is_hardware_control_volume = (volume_support_mask & ENDPOINT_HARDWARE_SUPPORT_VOLUME)
				&& (volume_support_mask & ENDPOINT_HARDWARE_SUPPORT_MUTE);

			// Exclusive mode device always support DSD
			info.is_support_dsd = true;

			device_list.push_back(info);
		} catch (const std::exception& e) {
			XAMP_LOG_D(logger_, "load device failed: {}", e.what());
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

ExclusiveWasapiDeviceType::ExclusiveWasapiDeviceType() : impl_(makeAlign<ExclusiveWasapiDeviceTypeImpl>()) {
}

void ExclusiveWasapiDeviceType::scanNewDevice() {
	impl_->scanNewDevice();
}

size_t ExclusiveWasapiDeviceType::getDeviceCount() const {
	return impl_->getDeviceCount();
}

DeviceInfo ExclusiveWasapiDeviceType::getDeviceInfo(uint32_t device) const {
	return impl_->getDeviceInfo(device);
}

std::optional<DeviceInfo> ExclusiveWasapiDeviceType::getDefaultDeviceInfo() const {
	return impl_->getDefaultDeviceInfo();
}

std::vector<DeviceInfo> ExclusiveWasapiDeviceType::getDeviceInfo() const {
	return impl_->getDeviceInfo();
}

ScopedPtr<IOutputDevice> ExclusiveWasapiDeviceType::makeDevice(const std::shared_ptr<IThreadPool>& thread_pool, std::string const& device_id) {
	return impl_->makeDevice(thread_pool, device_id);
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif
