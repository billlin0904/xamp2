#include <atlbase.h>
#include <initguid.h>

#include <output_device/win32/wasapi.h>

#ifdef XAMP_OS_WIN

#include <sstream>

#include <base/enum.h>
#include <base/logger.h>
#include <base/str_utilts.h>

#include <output_device/win32/comexception.h>

#include <devicetopology.h>
#include <functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>

XAMP_OUTPUT_DEVICE_WIN32_HELPER_NAMESPACE_BEGIN

namespace {
	using ComString = CComHeapPtr<wchar_t>;

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

	struct PropVariant final : PROPVARIANT {
		PropVariant() {
			::PropVariantInit(this);
		}

		XAMP_DISABLE_COPY(PropVariant)

		~PropVariant() {
			::PropVariantClear(this);
		}

		[[nodiscard]] std::wstring toString() const {
			std::wstring result;
			PWSTR psz = nullptr;
			if (SUCCEEDED(::PropVariantToStringAlloc(*this, &psz))) {
				result.assign(psz);
				::CoTaskMemFree(psz);
			}
			return result;
		}
	};

	DeviceConnectType getDeviceConnectType(const std::wstring& name) {
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

	DeviceConnectType getDeviceConnectType(CComPtr<IMMDevice>& device) {
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
		name = String::toLower(name);
		auto device_connect_type = getDeviceConnectType(name);
		XAMP_LOG_TRACE("EnumPartsIncoming: {} {}", device_connect_type, String::toString(name));
		return device_connect_type;
	}

	std::wstring getDevicePropertyString(const PROPERTYKEY& key, VARTYPE type, CComPtr<IMMDevice>& device) {
		std::wstring str;

		CComPtr<IPropertyStore> property;

		hrIfFailThrow(device->OpenPropertyStore(STGM_READ, &property));

		PropVariant prop_variant;

		hrIfFailThrow(property->GetValue(key, &prop_variant));

		switch (type) {
		case VT_UI4:
		{
			auto factor = static_cast<EndpointFactor>(prop_variant.ulVal);
			return String::toStdWString(enumToString(factor).data());
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
		return prop_variant.toString();
	}
}

CComPtr<IMMDeviceEnumerator> createDeviceEnumerator() {
	CComPtr<IMMDeviceEnumerator> enumerator;
	hrIfFailThrow(::CoCreateInstance(__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<void**>(&enumerator)));
	return enumerator;
}

DeviceInfo getDeviceInfo(CComPtr<IMMDevice>& device, const Uuid& device_type_id, std::string_view desc) {
	DeviceInfo info;
	info.name = getDevicePropertyString(PKEY_Device_FriendlyName, VT_LPWSTR, device);
	
	CComHeapPtr<WCHAR> id;
	hrIfFailThrow(device->GetId(&id));
	info.device_type_id = device_type_id;
	info.device_id = String::toUtf8String(std::wstring(id));
	info.connect_type = getDeviceConnectType(device);
	info.desc = desc;

	return info;
}

double getStreamPosInMilliseconds(CComPtr<IAudioClock>& clock) {
	UINT64 device_frequency = 0, position = 0;
	if (FAILED(clock->GetFrequency(&device_frequency)) ||
		FAILED(clock->GetPosition(&position, nullptr))) {
		return 0.0;
	}
	return 1000.0 * (static_cast<double>(position) / device_frequency);
}

AudioFormat toAudioFormat(const WAVEFORMATEX* format) {
	return AudioFormat(DataFormat::FORMAT_PCM, format->nChannels, format->wBitsPerSample, format->nSamplesPerSec);
}

bool isDeviceSupportExclusiveMode(const CComPtr<IMMDevice>& device, AudioFormat& default_format) {
	CComPtr<IPropertyStore> property;
	if (FAILED(device->OpenPropertyStore(STGM_READ, &property))) {
		return false;
	}

	PropVariant prop_variant;
	if (FAILED(property->GetValue(PKEY_AudioEngine_DeviceFormat, &prop_variant))) {
		return false;
	}

	if (prop_variant.vt != VT_BLOB
		|| prop_variant.blob.pBlobData == nullptr 
		|| prop_variant.blob.cbSize < sizeof(WAVEFORMATEX)) {
		return false;
	}

	auto* wfx = reinterpret_cast<PWAVEFORMATEX>(prop_variant.blob.pBlobData);
	const size_t base = sizeof(WAVEFORMATEX);
	const size_t total = base + static_cast<size_t>(wfx->cbSize);
	if (prop_variant.blob.cbSize < total) {
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
			wfx,
			nullptr
		);
		if (SUCCEEDED(hr)) {
			default_format = toAudioFormat(reinterpret_cast<WAVEFORMATEX*>(prop_variant.blob.pBlobData));
			return true;
		}
	}
	return false;
}

XAMP_OUTPUT_DEVICE_WIN32_HELPER_NAMESPACE_END

#endif

