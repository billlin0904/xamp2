#ifdef _WIN32
#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/win32/sharedwasapidevicetype.h>
#else
#include <output_device/osx/coreaudiodevicetype.h>
#endif

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
#if ENABLE_ASIO
	XAMP_REGISTER_DEVICE_TYPE(ASIODeviceType);
#endif
#else
    using namespace osx;
    XAMP_REGISTER_DEVICE_TYPE(CoreAudioDeviceType);
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

bool DeviceFactory::IsSupportedASIO() const {
#if ENABLE_ASIO && defined(_WIN32)
	return creator_.find(ASIODeviceType::Id) != creator_.end();
#else
	return false;
#endif
}

bool DeviceFactory::IsExclusiveDevice(const DeviceInfo& info) {
#ifdef _WIN32
	return info.device_type_id == win32::ExclusiveWasapiDeviceType::Id;
#else
    return false;
#endif
}

bool DeviceFactory::IsASIODevice(const ID id) {
#ifdef _WIN32
	return id == ASIODeviceType::Id;
#else
	return false;
#endif
}

}
