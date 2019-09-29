#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/win32/sharedwasapidevicetype.h>
#include <output_device/asiodevicetype.h>
#include <output_device/devicefactory.h>

namespace xamp::output_device {

#define XAMP_REGISTER_DEVICE_TYPE(DeviceTypeClass) \
	creator_.insert(std::make_pair(DeviceTypeClass::Id, []() {\
		return MakeAlign<DeviceType, DeviceTypeClass>();\
	}))

DeviceFactory::DeviceFactory() {
	using namespace win32;
	InitialDevice();
	XAMP_REGISTER_DEVICE_TYPE(SharedWasapiDeviceType);
	XAMP_REGISTER_DEVICE_TYPE(ExclusiveWasapiDeviceType);
	XAMP_REGISTER_DEVICE_TYPE(ASIODeviceType);
}

DeviceFactory::~DeviceFactory() {
	UnInitialDevice();
}

bool DeviceFactory::IsExclusiveDevice(const DeviceInfo& info) {
	return info.device_type_id == win32::ExclusiveWasapiDeviceType::Id;
}

}
