#include <output_device/win32/xaudio2devicetype.h>

#include <output_device/win32/wasapi.h>
#include <output_device/win32/comexception.h>
#include <output_device/win32/xaudio2outputdevice.h>

#include <atlbase.h>
#include <xaudio2.h>

#include <base/logger.h>
#include <base/str_utilts.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

class XAudio2DeviceType::XAudio2DeviceTypeImpl final {
public:
	XAudio2DeviceTypeImpl() ;

	~XAudio2DeviceTypeImpl();

	void scanNewDevice();

	[[nodiscard]] size_t getDeviceCount() const;

	[[nodiscard]] DeviceInfo getDeviceInfo(uint32_t device) const;

	[[nodiscard]] std::optional<DeviceInfo> getDefaultDeviceInfo() const;

	[[nodiscard]] std::vector<DeviceInfo> getDeviceInfo() const;

	ScopedPtr<IOutputDevice> makeDevice(const std::shared_ptr<IThreadPool>& thread_pool, const std::string& device_id);

private:
	[[nodiscard]] std::vector<DeviceInfo> GetDeviceInfoList() const;

	CComPtr<IMMDeviceEnumerator> enumerator_;
	std::vector<DeviceInfo> device_list_;
	LoggerPtr logger_;
};

XAudio2DeviceType::XAudio2DeviceTypeImpl::XAudio2DeviceTypeImpl() {
	logger_ = XampLoggerFactory.getLogger(XAMP_LOG_NAME(XAudio2DeviceType));
}

XAudio2DeviceType::XAudio2DeviceTypeImpl::~XAudio2DeviceTypeImpl() = default;

void XAudio2DeviceType::XAudio2DeviceTypeImpl::scanNewDevice() {
	enumerator_ = helper::CreateDeviceEnumerator();
	device_list_ = GetDeviceInfoList();
}

ScopedPtr<IOutputDevice> XAudio2DeviceType::XAudio2DeviceTypeImpl::makeDevice(const std::shared_ptr<IThreadPool>& thread_pool, const std::string& device_id) {
	return makeAlign<IOutputDevice, XAudio2OutputDevice>(thread_pool, String::toStdWString(device_id));
}

DeviceInfo XAudio2DeviceType::XAudio2DeviceTypeImpl::getDeviceInfo(uint32_t device) const {
	auto itr = device_list_.begin();
	if (device >= getDeviceCount()) {
		throw DeviceNotFoundException();
	}
	std::advance(itr, device);
	return (*itr);
}

size_t XAudio2DeviceType::XAudio2DeviceTypeImpl::getDeviceCount() const {
	return device_list_.size();
}

std::vector<DeviceInfo> XAudio2DeviceType::XAudio2DeviceTypeImpl::getDeviceInfo() const {
	return device_list_;
}

std::optional<DeviceInfo> XAudio2DeviceType::XAudio2DeviceTypeImpl::getDefaultDeviceInfo() const {
	CComPtr<IMMDevice> default_output_device;
	auto hr = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &default_output_device);
	constexpr auto kNotFoundHr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	HrIfNotEqualThrow(hr, kNotFoundHr);
	if (hr == kNotFoundHr) {
		return std::nullopt;
	}
	return makeOptional<DeviceInfo>(helper::getDeviceInfo(default_output_device,
		XAMP_UUID_OF(XAudio2DeviceType),
		XAudio2DeviceType::Description));
}

std::vector<DeviceInfo> XAudio2DeviceType::XAudio2DeviceTypeImpl::GetDeviceInfoList() const {
	std::vector<DeviceInfo> device_list;
	CComPtr<IMMDeviceCollection> devices;
	UINT count = 0;
	std::wstring default_device_name;

	try {
		// Get all active devices
		hrIfFailThrow(enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices));

		// Get device count
		hrIfFailThrow(devices->GetCount(&count));

		device_list.reserve(count);

		if (const auto default_device_info = getDefaultDeviceInfo()) {
			default_device_name = default_device_info.value().name;
		}
	}
	catch (const std::exception& e) {
		XAMP_LOG_E(logger_, "Fail to list active device! {}", e.what());
		return device_list;
	}

	XAMP_LOG_D(logger_, "load all devices");

	for (UINT i = 0; i < count; ++i) {
		CComPtr<IMMDevice> device;

		try {
			hrIfFailThrow(devices->Item(i, &device));

			auto info = helper::getDeviceInfo(device, XAMP_UUID_OF(XAudio2DeviceType), XAudio2DeviceType::Description);
			if (default_device_name == info.name) {
				info.is_default_device = true;
			}

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

			// Shared mode device always support hardware volume control
			info.is_hardware_control_volume = true;
			// Shared mode device always not support DSD
			info.is_support_dsd = false;
			device_list.push_back(info);
		}
		catch (const std::exception& e) {
			XAMP_LOG_D(logger_, "load device failed: {}", e.what());
		}
	}

	std::ranges::sort(device_list,
		[](const auto& first, const auto& second) {
			return first.name.length() > second.name.length();
		});

	return device_list;
}

XAMP_PIMPL_IMPL(XAudio2DeviceType)

XAudio2DeviceType::XAudio2DeviceType()
	: impl_(makeAlign<XAudio2DeviceTypeImpl>()) {
}

void XAudio2DeviceType::scanNewDevice() {
	impl_->scanNewDevice();
}

size_t XAudio2DeviceType::getDeviceCount() const {
	return impl_->getDeviceCount();
}

DeviceInfo XAudio2DeviceType::getDeviceInfo(uint32_t device) const {
	return impl_->getDeviceInfo(device);
}

std::optional<DeviceInfo> XAudio2DeviceType::getDefaultDeviceInfo() const {
	return impl_->getDefaultDeviceInfo();
}

std::vector<DeviceInfo> XAudio2DeviceType::getDeviceInfo() const {
	return impl_->getDeviceInfo();
}

ScopedPtr<IOutputDevice> XAudio2DeviceType::makeDevice(const std::shared_ptr<IThreadPool>& thread_pool, const std::string& device_id) {
	return impl_->makeDevice(thread_pool, device_id);
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END
