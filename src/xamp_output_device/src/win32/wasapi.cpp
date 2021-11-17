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
#include <output_device/idevicetype.h>
#include <output_device/win32/hrexception.h>
#include <output_device/win32/wasapi.h>

#include <propvarutil.h>

namespace xamp::output_device::win32::helper {

MAKE_XAMP_ENUM(EndpointFactor,
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
	PropVariant() noexcept { 
		::PropVariantInit(this);
	}

	~PropVariant() noexcept {
		::PropVariantClear(this);
	}

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

static DeviceConnectType GetDeviceConnectType(CComPtr<IMMDevice>& device) {
#define IfFailedRetrunUknownType(hr) \
	if (FAILED(hr)) {\
	return DeviceConnectType::UKNOWN;\
	}

	CComPtr<IDeviceTopology> device_topology;
	IfFailedRetrunUknownType(device->Activate(__uuidof(IDeviceTopology),
		CLSCTX_ALL,
		nullptr,
		reinterpret_cast<void**>(&device_topology)));

	CComPtr<IConnector> connector;
	IfFailedRetrunUknownType(device_topology->GetConnector(0, &connector));

	CComPtr<IPart> part;
	IfFailedRetrunUknownType(connector->QueryInterface(IID_PPV_ARGS(&part)));

	UINT id = 0;
	IfFailedRetrunUknownType(part->GetLocalId(&id));

	LPWSTR part_name = nullptr;
	IfFailedRetrunUknownType(part->GetName(&part_name));

	XAMP_ON_SCOPE_EXIT({
		::CoTaskMemFree(part_name);
		});
	std::wstring name(part_name);
	CComPtr<IPartsList> parts_list;
	auto hr= part->EnumPartsIncoming(&parts_list);
	if (hr == E_NOTFOUND) {
		CComPtr<IConnector> part_connector;
		IfFailedRetrunUknownType(part->QueryInterface(IID_PPV_ARGS(&part_connector)));
		CComPtr<IConnector> otherside_connector;
		IfFailedRetrunUknownType(part_connector->GetConnectedTo(&otherside_connector));
		CComPtr<IPart> otherside_part;
		IfFailedRetrunUknownType(otherside_connector->QueryInterface(IID_PPV_ARGS(&otherside_part)));
		CComPtr<IDeviceTopology> otherside_topology;
		IfFailedRetrunUknownType(otherside_part->GetTopologyObject(&otherside_topology));
		LPWSTR device_name = nullptr;
		IfFailedRetrunUknownType(otherside_topology->GetDeviceId(&device_name));
		XAMP_ON_SCOPE_EXIT({
			::CoTaskMemFree(device_name);
			});
		name = device_name;
	}
	if (name.find(L"usb") != std::wstring::npos) {
		return DeviceConnectType::USB;
	}
	if (name.find(L"hdaudio") != std::wstring::npos) {
		return DeviceConnectType::ON_BOARD;
	}
	XAMP_LOG_TRACE("EnumPartsIncoming: {} {}", id, String::ToString(name));
	return DeviceConnectType::UKNOWN;
}

CComPtr<IMMDeviceEnumerator> CreateDeviceEnumerator() {
	CComPtr<IMMDeviceEnumerator> enumerator;
	HrIfFailledThrow(::CoCreateInstance(__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<void**>(&enumerator)));
	return enumerator;
}

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

HashMap<std::string, std::wstring> GetDeviceProperty(CComPtr<IMMDevice>& device) {
	struct DeviceProperty {
		PROPERTYKEY key;
		VARTYPE type;
		std::string name;
	};

	HashMap<std::string, std::wstring> result;
	
	auto const device_property = std::vector<DeviceProperty>{
		{PKEY_AudioEndpoint_FormFactor , VT_UI4, "PKEY_AudioEndpoint_FormFactor"},
		{PKEY_AudioEndpoint_GUID,  VT_LPWSTR, "PKEY_AudioEndpoint_GUID"},
		{PKEY_AudioEngine_DeviceFormat,  VT_BLOB, "PKEY_AudioEngine_DeviceFormat"},
		{PKEY_Device_EnumeratorName,  VT_LPWSTR, "PKEY_Device_EnumeratorName"},
		{PKEY_AudioEndpoint_JackSubType,  VT_LPWSTR, "PKEY_AudioEndpoint_JackSubType"},
		{PKEY_Device_EnumeratorName,  VT_LPWSTR, "PKEY_Device_EnumeratorName"},
	};

	for (const auto &property : device_property) {
		result[property.name] = GetDevicePropertyString(property.key, property.type, device);
	}

	return result;
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



}
#endif
