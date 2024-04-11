#include <output_device/win32/xaudio2devicetype.h>

#include <output_device/win32/wasapi.h>
#include <output_device/win32/comexception.h>
#include <output_device/win32/wasapi.h>
#include <output_device/win32/xaudio2outputdevice.h>

#include <xaudio2.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/str_utilts.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

class XAudio2DeviceType::XAudio2DeviceTypeImpl final {
public:
	XAudio2DeviceTypeImpl() noexcept;

	void ScanNewDevice();

	[[nodiscard]] size_t GetDeviceCount() const;

	[[nodiscard]] DeviceInfo GetDeviceInfo(uint32_t device) const;

	[[nodiscard]] std::optional<DeviceInfo> GetDefaultDeviceInfo() const;

	[[nodiscard]] Vector<DeviceInfo> GetDeviceInfo() const;

	AlignPtr<IOutputDevice> MakeDevice(const std::string& device_id);

private:
	[[nodiscard]] Vector<DeviceInfo> GetDeviceInfoList() const;

	CComPtr<IMMDeviceEnumerator> enumerator_;
	Vector<DeviceInfo> device_list_;
	LoggerPtr logger_;
};

XAudio2DeviceType::XAudio2DeviceTypeImpl::XAudio2DeviceTypeImpl() noexcept {
	logger_ = XampLoggerFactory.GetLogger(kXAudio2DeviceTypeLoggerName);
}

void XAudio2DeviceType::XAudio2DeviceTypeImpl::ScanNewDevice() {
	enumerator_ = helper::CreateDeviceEnumerator();
	device_list_ = GetDeviceInfoList();
}

AlignPtr<IOutputDevice> XAudio2DeviceType::XAudio2DeviceTypeImpl::MakeDevice(const std::string& device_id) {
	CComPtr<IXAudio2> xaudio2;

	UINT32 flags = 0;
	HrIfFailThrow(::XAudio2Create(&xaudio2, XAUDIO2_DEBUG_ENGINE, XAUDIO2_DEFAULT_PROCESSOR));

	XAUDIO2_DEBUG_CONFIGURATION debug_config;
	debug_config.TraceMask = XAUDIO2_LOG_WARNINGS | XAUDIO2_LOG_DETAIL | XAUDIO2_LOG_FUNC_CALLS | XAUDIO2_LOG_TIMING | XAUDIO2_LOG_LOCKS | XAUDIO2_LOG_MEMORY | XAUDIO2_LOG_STREAMING;
	debug_config.BreakMask = XAUDIO2_LOG_WARNINGS;
	debug_config.LogThreadID = TRUE;
	debug_config.LogFileline = TRUE;
	debug_config.LogFunctionName = TRUE;
	debug_config.LogTiming = TRUE;
	xaudio2->SetDebugConfiguration(&debug_config, nullptr);

	return MakeAlign<IOutputDevice, XAudio2OutputDevice>(xaudio2, String::ToStdWString(device_id));
}

DeviceInfo XAudio2DeviceType::XAudio2DeviceTypeImpl::GetDeviceInfo(uint32_t device) const {
	auto itr = device_list_.begin();
	if (device >= GetDeviceCount()) {
		throw DeviceNotFoundException();
	}
	std::advance(itr, device);
	return (*itr);
}

size_t XAudio2DeviceType::XAudio2DeviceTypeImpl::GetDeviceCount() const {
	return device_list_.size();
}

Vector<DeviceInfo> XAudio2DeviceType::XAudio2DeviceTypeImpl::GetDeviceInfo() const {
	return device_list_;
}

std::optional<DeviceInfo> XAudio2DeviceType::XAudio2DeviceTypeImpl::GetDefaultDeviceInfo() const {
	CComPtr<IMMDevice> default_output_device;
	auto hr = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &default_output_device);
	HrIfNotEqualThrow(hr, ERROR_NOT_FOUND);
	if (hr == ERROR_NOT_FOUND) {
		return std::nullopt;
	}
	return std::optional<DeviceInfo> { std::in_place_t{}, helper::GetDeviceInfo(default_output_device, XAMP_UUID_OF(XAudio2DeviceType)) };
}

Vector<DeviceInfo> XAudio2DeviceType::XAudio2DeviceTypeImpl::GetDeviceInfoList() const {
	Vector<DeviceInfo> device_list;
	CComPtr<IMMDeviceCollection> devices;
	UINT count = 0;
	std::wstring default_device_name;

	try {
		// Get all active devices
		HrIfFailThrow(enumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices));

		// Get device count
		HrIfFailThrow(devices->GetCount(&count));

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
			HrIfFailThrow(devices->Item(i, &device));

			auto info = helper::GetDeviceInfo(device, XAMP_UUID_OF(XAudio2DeviceType));
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
			XAMP_LOG_D(logger_, "Load device failed: {}", e.what());
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
	: impl_(MakeAlign<XAudio2DeviceTypeImpl>()) {
}

void XAudio2DeviceType::ScanNewDevice() {
	impl_->ScanNewDevice();
}

std::string_view XAudio2DeviceType::GetDescription() const {
	return Description;
}

Uuid XAudio2DeviceType::GetTypeId() const {
	return XAMP_UUID_OF(XAudio2DeviceType);
}

size_t XAudio2DeviceType::GetDeviceCount() const {
	return impl_->GetDeviceCount();
}

DeviceInfo XAudio2DeviceType::GetDeviceInfo(uint32_t device) const {
	return impl_->GetDeviceInfo(device);
}

std::optional<DeviceInfo> XAudio2DeviceType::GetDefaultDeviceInfo() const {
	return impl_->GetDefaultDeviceInfo();
}

Vector<DeviceInfo> XAudio2DeviceType::GetDeviceInfo() const {
	return impl_->GetDeviceInfo();
}

AlignPtr<IOutputDevice> XAudio2DeviceType::MakeDevice(const std::string& device_id) {
	return impl_->MakeDevice(device_id);
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END
