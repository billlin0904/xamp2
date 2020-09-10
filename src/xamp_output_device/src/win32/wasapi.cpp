#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/str_utilts.h>
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
	::PropVariantInit(&prop_variant);

	HrIfFailledThrow(property->GetValue(key, &prop_variant));

	std::wstring name;
	name.assign(prop_variant.pwszVal);

	::PropVariantClear(&prop_variant);

	return name;
}

DeviceInfo GetDeviceInfo(CComPtr<IMMDevice>& device, ID const& device_type_id) {
	DeviceInfo info;
	info.name = GetDeviceProperty(PKEY_Device_FriendlyName, device);

	CComHeapPtr<WCHAR> id;
	HrIfFailledThrow(device->GetId(&id));
	info.device_type_id = device_type_id;
	info.device_id = ToUtf8String(std::wstring(id));

	return info;
}

}
#endif
