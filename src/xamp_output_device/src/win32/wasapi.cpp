#include <sstream>
#include <base/base.h>
#include <base/enum.h>

#ifdef XAMP_OS_WIN

#include <initguid.h>
#include <cguid.h>

#include <base/stl.h>
#include <base/str_utilts.h>
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

	std::wstring ToString() noexcept {
		std::wstring result;
		PWSTR psz = nullptr;
		if (SUCCEEDED(::PropVariantToStringAlloc(*this, &psz))) {
			result.assign(psz);
			::CoTaskMemFree(psz);
		}
		return result;
	}
};

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
		return ToStdWString(EnumToString(factor).data());
	}
		break;
	case VT_BLOB:
	{
		std::wostringstream ostr;
		auto format = reinterpret_cast<const WAVEFORMATEX*>(prop_variant.blob.pBlobData);
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
	info.device_id = ToUtf8String(std::wstring(id));

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
