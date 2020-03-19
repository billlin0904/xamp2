#ifdef _WIN32
#include <output_device/win32/hrexception.h>
#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/win32/sharedwasapidevicetype.h>
#else
#include <output_device/osx/coreaudiodevicetype.h>
#endif

#if ENABLE_ASIO
#include <output_device/asiodevicetype.h>
#endif

#include <output_device/devicefactory.h>

namespace xamp::output_device {

#define XAMP_REGISTER_DEVICE_TYPE(DeviceTypeClass) \
	creator_.insert(std::make_pair(DeviceTypeClass::Id, []() {\
		return MakeAlign<DeviceType, DeviceTypeClass>();\
	}))

DeviceFactory::DeviceFactory() {
#ifdef _WIN32
	using namespace win32;	
	HrIfFailledThrow(::MFStartup(MF_VERSION, MFSTARTUP_LITE));
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
#ifdef _WIN32
	::MFShutdown();
#endif
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

bool DeviceFactory::IsPlatformSupportedASIO() const {
#if ENABLE_ASIO && defined(_WIN32)
	return creator_.find(ASIODeviceType::Id) != creator_.end();
#else
	return false;
#endif
}

bool DeviceFactory::IsExclusiveDevice(const DeviceInfo& info) {
#ifdef _WIN32
	return info.device_type_id == win32::ExclusiveWasapiDeviceType::Id
#if ENABLE_ASIO
		|| info.device_type_id == ASIODeviceType::Id
#endif
	;
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

bool DeviceFactory::IsDeviceTypeExist(const ID id) const {
	return creator_.find(id) != creator_.end();
}

void DeviceFactory::RegisterDeviceListener(std::weak_ptr<DeviceStateListener> callback) {
#ifdef _WIN32
	notification_ = new win32::DeviceStateNotification(callback);
#else
	notification_ = MakeAlign<osx::DeviceStateNotification>(callback);
#endif
	notification_->Run();
}

}
