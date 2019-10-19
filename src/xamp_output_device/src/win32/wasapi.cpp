#ifdef _WIN32
#include <output_device/win32/hrexception.h>
#include <output_device/win32/wasapi.h>

namespace xamp::output_device::win32::helper {

std::wstring GetDeviceProperty(const PROPERTYKEY& key, CComPtr<IMMDevice>& device) {
	CComPtr<IPropertyStore> property;

	HR_IF_FAILED_THROW(device->OpenPropertyStore(STGM_READ, &property));

	PROPVARIANT prop_variant;
	PropVariantInit(&prop_variant);

	HR_IF_FAILED_THROW(property->GetValue(key, &prop_variant));

	std::wstring name;
	name.assign(prop_variant.pwszVal);

	PropVariantClear(&prop_variant);

	return name;
}

DeviceInfo GetDeviceInfo(CComPtr<IMMDevice>& device, const ID device_type_id) {
	DeviceInfo info;
	info.name = GetDeviceProperty(PKEY_Device_FriendlyName, device);

	CComHeapPtr<WCHAR> id;
	HR_IF_FAILED_THROW(device->GetId(&id));
	info.device_type_id = device_type_id;
	info.device_id = id;

	return info;
}

}
#endif
