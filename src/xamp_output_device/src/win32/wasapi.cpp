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
			return DeviceConnectType::CONNECT_TYPE_USB;
		}
		if (name.find(L"hdaudio") != std::wstring::npos) {
			return DeviceConnectType::CONNECT_TYPE_BUILT_IN;
		}
		if (name.find(L"bthenum") != std::wstring::npos) {
			return DeviceConnectType::CONNECT_TYPE_BLUE_TOOTH;
		}
		return DeviceConnectType::CONNECT_TYPE_UNKNOWN;
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
			return DeviceConnectType::CONNECT_TYPE_UNKNOWN;\
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
	std::wstring GetDevicePropertyString(PROPERTYKEY const& key, VARTYPE type, CComPtr<IMMDevice>& device) {
		std::wstring str;

		CComPtr<IPropertyStore> property;

		HrIfFailledThrow(device->OpenPropertyStore(STGM_READ, &property));

		PropVariant prop_variant;

		HrIfFailledThrow(property->GetValue(key, &prop_variant));

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
	HrIfFailledThrow(::CoCreateInstance(__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<void**>(&enumerator)));
	return enumerator;
}

DeviceInfo GetDeviceInfo(CComPtr<IMMDevice>& device, Uuid const& device_type_id) {
	DeviceInfo info;
	info.name = GetDevicePropertyString(PKEY_Device_FriendlyName, VT_LPWSTR, device);
	
	CComHeapPtr<WCHAR> id;
	HrIfFailledThrow(device->GetId(&id));
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

XAMP_OUTPUT_DEVICE_WIN32_HELPER_NAMESPACE_END

#endif // XAMP_OS_WIN

