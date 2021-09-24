#include <sstream>
#include <base/base.h>
#include <base/enum.h>

#ifdef XAMP_OS_WIN

#include <initguid.h>
#include <cguid.h>

#include <base/stl.h>
#include <base/scopeguard.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <output_device/device_type.h>
#include <output_device/win32/hrexception.h>
#include <output_device/win32/wasapi.h>

#include <propvarutil.h>

namespace xamp::output_device::win32::helper {

MAKE_ENUM(EndpointFactor,
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

#define IfFailedRetrun(hr) \
	if (FAILED(hr)) {\
		return DeviceConnectType::UKNOWN;\
	}

static DeviceConnectType GetDeviceConnectType(CComPtr<IMMDevice>& device) {
	CComPtr<IDeviceTopology> device_topology;
	HrIfFailledThrow(device->Activate(__uuidof(IDeviceTopology),
		CLSCTX_ALL,
		nullptr,
		reinterpret_cast<void**>(&device_topology)));

	CComPtr<IConnector> connector;
	HrIfFailledThrow(device_topology->GetConnector(0, &connector));

	CComPtr<IPart> part;
	HrIfFailledThrow(connector->QueryInterface(IID_PPV_ARGS(&part)));

	UINT id = 0;
	HrIfFailledThrow(part->GetLocalId(&id));

	LPWSTR part_name = nullptr;
	HrIfFailledThrow(part->GetName(&part_name));

	XAMP_ON_SCOPE_EXIT({
		::CoTaskMemFree(part_name);
		});
	std::wstring name(part_name);
	XAMP_LOG_DEBUG("{}: {}", id, String::ToString(name));
	CComPtr<IPartsList> parts_list;
	auto hr= part->EnumPartsIncoming(&parts_list);
	if (hr == E_NOTFOUND) {
		CComPtr<IConnector> part_connector;
		IfFailedRetrun(part->QueryInterface(IID_PPV_ARGS(&part_connector)));
		CComPtr<IConnector> otherside_connector;
		IfFailedRetrun(part_connector->GetConnectedTo(&otherside_connector));
		CComPtr<IPart> otherside_part;
		IfFailedRetrun(otherside_connector->QueryInterface(IID_PPV_ARGS(&otherside_part)));
		CComPtr<IDeviceTopology> otherside_topology;
		IfFailedRetrun(otherside_part->GetTopologyObject(&otherside_topology));
		LPWSTR device_name = nullptr;
		IfFailedRetrun(otherside_topology->GetDeviceId(&device_name));
		XAMP_ON_SCOPE_EXIT({
			::CoTaskMemFree(device_name);
			});
		name = device_name;
		XAMP_LOG_DEBUG("EnumPartsIncoming: {} {}", id, String::ToString(name));
	}
	if (name.find(L"usb") != std::wstring::npos) {
		return DeviceConnectType::USB;
	}
	if (name.find(L"hdaudio") != std::wstring::npos) {
		return DeviceConnectType::ON_BOARD;
	}
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
		const auto* format = reinterpret_cast<const WAVEFORMATEX*>(prop_variant.blob.pBlobData);
		if (format != nullptr) {
			ostr << format->nChannels << "," << format->wBitsPerSample << "," << format->nSamplesPerSec;
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
