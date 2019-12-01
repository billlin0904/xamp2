#ifdef _WIN32
#include <output_device/win32/hrexception.h>
#endif

#include <output_device/output_device.h>

namespace xamp::output_device {

void InitialDevice() {
#ifdef _WIN32
	using namespace xamp::output_device::win32;
	HrIfFailledThrow(MFStartup(MF_VERSION, MFSTARTUP_LITE));	
#endif
}

void UnInitialDevice() {	
#ifdef _WIN32
	MFShutdown();
#endif
}

}
