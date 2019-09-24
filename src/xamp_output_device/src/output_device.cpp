#include <output_device/output_device.h>
#include <output_device/devicefactory.h>

#ifdef _WIN32
#include <output_device/win32/hrexception.h>
#include <output_device/win32/sharedwasapidevicetype.h>
#include <output_device/win32/exclusivewasapidevicetype.h>
#endif

namespace xamp::output_device {

using namespace win32;

void InitialDevice() {
#ifdef _WIN32
	HR_IF_FAILED_THROW(MFStartup(MF_VERSION, MFSTARTUP_LITE));
	XAMP_REGISTER_DEVICE_TYPE(SharedWasapiDeviceType);
	XAMP_REGISTER_DEVICE_TYPE(ExclusiveWasapiDeviceType);
#endif
}

void UnInitialDevice() {	
#ifdef _WIN32
	MFShutdown();
#endif
	DeviceFactory::Instance().Clear();
}

}
