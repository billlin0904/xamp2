#include <output_device/output_device.h>
#include <output_device/devicefactory.h>

#include <output_device/win32/sharedwasapidevicetype.h>
#include <output_device/win32/exclusivewasapidevicetype.h>

namespace xamp::output_device {

using namespace win32;

void InitialDevice() {
	XAMP_REGISTER_DEVICE_TYPE(SharedWasapiDeviceType);
	XAMP_REGISTER_DEVICE_TYPE(ExclusiveWasapiDeviceType);
}

void UnInitialDevice() {
	DeviceFactory::Instance().Clear();
}

}
