#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/win32/wasapi.h>
#include <output_device/win32/hrexception.h>
#include <output_device/win32/wasapi.h>
#include <output_device/win32/sharedwasapidevice.h>
#include <output_device/win32/sharedwasapidevicetype.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/str_utilts.h>

namespace xamp::output_device::win32 {

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
	void Initial();

	[[nodiscard]] Vector<DeviceInfo> GetDeviceInfoList() const;

	[[nodiscard]] CComPtr<IMMDevice> GetDeviceById(const std::wstring& device_id) const;

	LoggerPtr log_;
	Vector<DeviceInfo> device_list_;
	CComPtr<IMMDeviceEnumerator> enumerator_;
};

SharedWasapiDeviceType::SharedWasapiDeviceTypeImpl::SharedWasapiDeviceTypeImpl() noexcept {
	log_ = LoggerManager::GetInstance().GetLogger(kSharedWasapiDeviceLoggerName);
	ScanNewDevice();
}

void SharedWasapiDeviceType::SharedWasapiDeviceTypeImpl::ScanNewDevice() {
	Initial();
	device_list_ = GetDeviceInfoList();
}

void SharedWasapiDeviceType::SharedWasapiDeviceTypeImpl::Initial() {
	if (!enumerator_) {
		enumerator_ = helper::CreateDeviceEnumerator();
	}
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

		auto info = helper::GetDeviceInfo(device, XAMP_UUID_OF(SharedWasapiDeviceType));
#ifdef _DEBUG
		XAMP_LOG_TRACE("Get {} device {} property.", SharedWasapiDeviceType::Description, String::ToUtf8String(info.name));
		for (const auto& property : helper::GetDeviceProperty(device)) {
			XAMP_LOG_TRACE("{}: {}", property.first, String::ToUtf8String(property.second));
		}
#endif  
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

		XAMP_LOG_D(log_, "{:30} min_volume: {:.2f} dBFS, max_volume:{:.2f} dBFS, volume_increnment:{:.2f} dBFS, volume leve:{:.2f}.",
			String::ToUtf8String(info.name),
			info.min_volume,
			info.max_volume,
			info.volume_increment,
			(info.max_volume - info.min_volume) / info.volume_increment);

		info.is_hardware_control_volume = true;

		info.is_support_dsd = false;
		device_list.emplace_back(info);
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

}
#endif

