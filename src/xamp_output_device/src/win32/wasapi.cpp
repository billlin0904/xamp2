#include <sstream>
#include <base/base.h>
#include <base/enum.h>

#ifdef XAMP_OS_WIN

#include <initguid.h>
#include <cguid.h> // GUID_NULL

#include <base/stl.h>
#include <base/scopeguard.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/platfrom_handle.h>
#include <output_device/idevicetype.h>
#include <output_device/win32/comexception.h>
#include <output_device/win32/wasapi.h>

#include <propvarutil.h>

XAMP_OUTPUT_DEVICE_WIN32_HELPER_NAMESPACE_BEGIN

namespace {
	XAMP_MAKE_ENUM(EndpointFactor,
		RemoteNetworkDevice,
		Speakers,
		LineLevel,
		Headphones,
		Microphone,
		Headset,
		Handset,
		UnknownDigitalPassthrough,
		SPDIF,
		DigitalAudioDisplayDevice,
		UnknownFormFactor);

	/*
	* Propvariant wrapper class.
	*
	*/
	struct PropVariant final : PROPVARIANT {
		/*
		* Constructor.
		*/
		PropVariant() noexcept {
			::PropVariantInit(this);
		}

		XAMP_DISABLE_COPY(PropVariant)

		/*
		* Destructor.
		*/
		~PropVariant() noexcept {
			::PropVariantClear(this);
		}

		/*
		* To string.
		*
		* @return std::wstring
		*/
		[[nodiscard]] std::wstring ToString() const noexcept {
			std::wstring result;
			PWSTR psz = nullptr;
			if (SUCCEEDED(::PropVariantToStringAlloc(*this, &psz))) {
				result.assign(psz);
				::CoTaskMemFree(psz);
			}
			return result;
		}
	};

	/*
	* Get device connect type.
	*
	* @param[in] name device name.
	* @return DeviceConnectType
	*/
	DeviceConnectType GetDeviceConnectType(const std::wstring& name) {
		if (name.find(L"usb") != std::wstring::npos) {
			return DeviceConnectType::USB;
		}
		if (name.find(L"hdaudio") != std::wstring::npos) {
			return DeviceConnectType::BUILT_IN_SPEAKER;
		}
		if (name.find(L"bthenum") != std::wstring::npos) {
			return DeviceConnectType::BLUE_TOOTH;
		}
		return DeviceConnectType::UNKNOWN;
	}

	/*
	 * Get device connect type.
	 *
	 * @param[in] device device.
	 * @return DeviceConnectType
	 */
	DeviceConnectType GetDeviceConnectType(CComPtr<IMMDevice>& device) {
#define IfFailedReturnUnknownType(hr) \
		if (FAILED(hr)) {\
			return DeviceConnectType::UNKNOWN;\
		}

		// Get device topology
		CComPtr<IDeviceTopology> device_topology;
		IfFailedReturnUnknownType(device->Activate(__uuidof(IDeviceTopology),
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&device_topology)))

		// Get connector
		CComPtr<IConnector> connector;
		IfFailedReturnUnknownType(device_topology->GetConnector(0, &connector))

		// Get part
		CComPtr<IPart> part;
		IfFailedReturnUnknownType(connector->QueryInterface(IID_PPV_ARGS(&part)))

		// Get part id
		UINT id = 0;
		IfFailedReturnUnknownType(part->GetLocalId(&id))

		// Get part name
		ComString part_name;
		IfFailedReturnUnknownType(part->GetName(&part_name))

		std::wstring name(part_name);
		CComPtr<IPartsList> parts_list;

		// Enum parts incoming
		auto hr = part->EnumPartsIncoming(&parts_list);
		if (hr == E_NOTFOUND) {
			// Enum parts outgoing
			CComPtr<IConnector> part_connector;
			IfFailedReturnUnknownType(part->QueryInterface(IID_PPV_ARGS(&part_connector)))
				// Get connected to
				CComPtr<IConnector> otherside_connector;
			IfFailedReturnUnknownType(part_connector->GetConnectedTo(&otherside_connector))
				// Get part
				CComPtr<IPart> otherside_part;
			IfFailedReturnUnknownType(otherside_connector->QueryInterface(IID_PPV_ARGS(&otherside_part)))
				// Get topology
				CComPtr<IDeviceTopology> otherside_topology;
			IfFailedReturnUnknownType(otherside_part->GetTopologyObject(&otherside_topology))
				// Get device id
				ComString device_name;
			IfFailedReturnUnknownType(otherside_topology->GetDeviceId(&device_name))
				name = device_name;
		}
		name = String::ToLower(name);
		auto device_connect_type = GetDeviceConnectType(name);
		XAMP_LOG_TRACE("EnumPartsIncoming: {} {}", device_connect_type, String::ToString(name));
		return device_connect_type;
	}

	/*
	 * Get device property string.
	 *
	 * @param[in] key property key.
	 * @param[in] type property type.
	 * @param[in] device device.
	 * @return std::wstring
	*/
	std::wstring GetDevicePropertyString(const PROPERTYKEY& key, VARTYPE type, CComPtr<IMMDevice>& device) {
		std::wstring str;

		CComPtr<IPropertyStore> property;

		HrIfFailThrow(device->OpenPropertyStore(STGM_READ, &property));

		PropVariant prop_variant;

		HrIfFailThrow(property->GetValue(key, &prop_variant));

		switch (type) {
		case VT_UI4:
		{
			auto factor = static_cast<EndpointFactor>(prop_variant.ulVal);
			return String::ToStdWString(EnumToString(factor).data());
		}
		break;
		case VT_BLOB:
		{
			std::wostringstream ostr;
			const auto* format = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(prop_variant.blob.pBlobData);
			if (format != nullptr) {
				ostr << format->Format.nChannels << "," << format->Format.wBitsPerSample << "," << format->Format.nSamplesPerSec;
			}
			return ostr.str();
		}
		break;
		}
		return prop_variant.ToString();
	}
}

/*
* Create device enumerator.
* 
* @return CComPtr<IMMDeviceEnumerator>
*/
CComPtr<IMMDeviceEnumerator> CreateDeviceEnumerator() {
	CComPtr<IMMDeviceEnumerator> enumerator;
	HrIfFailThrow(::CoCreateInstance(__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<void**>(&enumerator)));
	return enumerator;
}

DeviceInfo GetDeviceInfo(CComPtr<IMMDevice>& device, const Uuid& device_type_id) {
	DeviceInfo info;
	info.name = GetDevicePropertyString(PKEY_Device_FriendlyName, VT_LPWSTR, device);
	
	CComHeapPtr<WCHAR> id;
	HrIfFailThrow(device->GetId(&id));
	info.device_type_id = device_type_id;
	info.device_id = String::ToUtf8String(std::wstring(id));
	info.connect_type = GetDeviceConnectType(device);

	return info;
}

double GetStreamPosInMilliseconds(CComPtr<IAudioClock>& clock) {
	UINT64 device_frequency = 0, position = 0;
	if (FAILED(clock->GetFrequency(&device_frequency)) ||
		FAILED(clock->GetPosition(&position, nullptr))) {
		return 0.0;
	}
	return 1000.0 * (static_cast<double>(position) / device_frequency);
}

AudioFormat ToAudioFormat(const WAVEFORMATEX* format) {
	return AudioFormat(DataFormat::FORMAT_PCM, format->nChannels, format->wBitsPerSample, format->nSamplesPerSec);
}

bool IsDeviceSupportExclusiveMode(const CComPtr<IMMDevice>& device, AudioFormat& default_format) {
	//CComPtr<IAudioClient> client;
	//auto hr = device->Activate(__uuidof(IAudioClient),
	//	CLSCTX_ALL,
	//	nullptr,
	//	reinterpret_cast<void**>(&client));
	//if (FAILED(hr)) {
	//	return false;
	//}

	//REFERENCE_TIME default_device_period = 0;
	//REFERENCE_TIME minimum_device_period = 0;
	//if (FAILED(client->GetDevicePeriod(&default_device_period, &minimum_device_period))) {
	//	return false;
	//}

	//constexpr struct TestFormat {
	//	uint32_t sample_rate;
	//	uint16_t bits_per_sample;
	//} test_formats[] = {
	//	{44100, 24},  // 44.1kHz/24bit
	//	{44100, 32},  // 44.1kHz/32bit
	//	{48000, 24},  // 48kHz/24bit
	//	{48000, 32},  // 48kHz/32bit
	//	// 為了加速測試速度，僅測試以上格式.
	//};

	//for (auto test_format : test_formats) {
	//	const AudioFormat audio_format(DataFormat::FORMAT_PCM, AudioFormat::kMaxChannel, test_format.bits_per_sample, test_format.sample_rate);

	//	WAVEFORMATEXTENSIBLE format{ 0 };
	//	format.Format.nChannels = audio_format.GetChannels();
	//	format.Format.nSamplesPerSec = audio_format.GetSampleRate();
	//	format.Format.nAvgBytesPerSec = audio_format.GetAvgBytesPerSec();
	//	format.Format.nBlockAlign = audio_format.GetBlockAlign();
	//	format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
	//	format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	//	format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
	//	format.Samples.wValidBitsPerSample = test_formats->bits_per_sample;
	//	format.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;

	//	// 雖然有效位元是24或32，但WAVEFORMATEXTENSIBLE結構中Format.wBitsPerSample若設定32且wValidBitsPerSample=24表示24bit pack於32bit框架中
	//	format.Format.wBitsPerSample = 32;

	//	const auto* mix_format = reinterpret_cast<WAVEFORMATEX*>(&format);
	//	hr = client->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE,
	//		AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
	//		default_device_period,
	//		default_device_period,
	//		mix_format,
	//		nullptr);
	//	if (SUCCEEDED(hr)) {
	//		default_format = audio_format;
	//		return true;
	//	}
	//}

	//return false;

	CComPtr<IPropertyStore> property;
	if (FAILED(device->OpenPropertyStore(STGM_READ, &property))) {
		return false;
	}

	PropVariant prop_variant;
	if (FAILED(property->GetValue(PKEY_AudioEngine_DeviceFormat, &prop_variant))) {
		return false;
	}

	CComPtr<IAudioClient> client;
	auto hr = device->Activate(__uuidof(IAudioClient),
		CLSCTX_ALL,
		nullptr,
		reinterpret_cast<void**>(&client));
	if (SUCCEEDED(hr)) {
		hr = client->IsFormatSupported(
			AUDCLNT_SHAREMODE_EXCLUSIVE,
			reinterpret_cast<PWAVEFORMATEX>(prop_variant.blob.pBlobData),
			nullptr
		);
		if (SUCCEEDED(hr)) {
			default_format = ToAudioFormat(reinterpret_cast<WAVEFORMATEX*>(prop_variant.blob.pBlobData));
			return true;
		}
	}
	return false;
	return true;
}

XAMP_OUTPUT_DEVICE_WIN32_HELPER_NAMESPACE_END

#endif

