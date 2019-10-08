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
	InitialDevice();
#ifdef _WIN32
	using namespace win32;	
	XAMP_REGISTER_DEVICE_TYPE(SharedWasapiDeviceType);
	XAMP_REGISTER_DEVICE_TYPE(ExclusiveWasapiDeviceType);
	XAMP_REGISTER_DEVICE_TYPE(ASIODeviceType);
#endif
}

DeviceFactory::~DeviceFactory() {
	UnInitialDevice();
}

void DeviceFactory::Clear() {
	creator_.clear();
}

std::optional<AlignPtr<DeviceType>> DeviceFactory::CreateDefaultDevice() const {
	auto itr = creator_.begin();
	if (itr == creator_.end()) {
		return std::nullopt;
	}
	return (*itr).second();
}

std::optional<AlignPtr<DeviceType>> DeviceFactory::Create(const ID id) const {
	auto itr = creator_.find(id);
	if (itr == creator_.end()) {
		return std::nullopt;
	}
	return (*itr).second();
}

bool DeviceFactory::IsExclusiveDevice(const DeviceInfo& info) {
	return info.device_type_id == win32::ExclusiveWasapiDeviceType::Id;
}

}
