#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/devicefactory.h>

namespace xamp::output_device {

bool DeviceFactory::IsExclusiveDevice(const DeviceInfo& info) {
	return info.device_type_id == win32::ExclusiveWasapiDeviceType::Id;
}

}
