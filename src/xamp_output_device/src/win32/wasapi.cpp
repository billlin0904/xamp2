// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/win32/hrexception.h>
#include <output_device/win32/wasapi.h>

namespace xamp::output_device::win32::helper {

CComPtr<IMMDeviceEnumerator> CreateDeviceEnumerator() {
	CComPtr<IMMDeviceEnumerator> enumerator;
	HrIfFailledThrow(::CoCreateInstance(__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<void**>(&enumerator)));
	return enumerator;
}

std::wstring GetDeviceProperty(PROPERTYKEY const & key, CComPtr<IMMDevice>& device) {
	CComPtr<IPropertyStore> property;

	HrIfFailledThrow(device->OpenPropertyStore(STGM_READ, &property));

	PROPVARIANT prop_variant;
	PropVariantInit(&prop_variant);

	HrIfFailledThrow(property->GetValue(key, &prop_variant));

	std::wstring name;
	name.assign(prop_variant.pwszVal);

	PropVariantClear(&prop_variant);

	return name;
}

DeviceInfo GetDeviceInfo(CComPtr<IMMDevice>& device, ID device_type_id) {
	DeviceInfo info;
	info.name = GetDeviceProperty(PKEY_Device_FriendlyName, device);

	CComHeapPtr<WCHAR> id;
	HrIfFailledThrow(device->GetId(&id));
	info.device_type_id = device_type_id;
	info.device_id = id;

	return info;
}

}
#endif
