#include <output_device/devicefactory.h>
#include <output_device/asiodevicetype.h>

#ifdef _WIN32
#include <output_device/win32/hrexception.h>
#include <output_device/win32/sharedwasapidevicetype.h>
#include <output_device/win32/exclusivewasapidevicetype.h>
#endif

#include <output_device/output_device.h>

namespace xamp::output_device {

using namespace win32;

void InitialDevice() {
#ifdef _WIN32
	HR_IF_FAILED_THROW(MFStartup(MF_VERSION, MFSTARTUP_LITE));	
#endif
}

void UnInitialDevice() {	
#ifdef _WIN32
	MFShutdown();
#endif
}

}
