#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <output_device/win32/hrexception.h>
#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/win32/sharedwasapidevicetype.h>
#include <output_device/win32/win32devicestatenotification.h>
#else
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <output_device/osx/coreaudiodevicetype.h>
#include <output_device/osx/coreaudiodevicestatenotification.h>
#endif

#include <base/platform_thread.h>

#if ENABLE_ASIO
#include <output_device/asiodevicetype.h>
#endif

#include <output_device/devicefactory.h>

namespace xamp::output_device {

#ifdef XAMP_OS_MAC
static void AwakeFromDisplaySleep() {
    CFTimeInterval timeout = 5;
    IOPMAssertionID id = 0;
    IOReturn result = ::IOPMAssertionCreateWithDescription(kIOPMAssertionTypePreventUserIdleSystemSleep,
                                                           CFSTR("XAMP"),
                                                           CFSTR("XAMP"),
                                                           CFSTR("Avoid player playing into sleep"),
                                                           CFSTR("/System/Library/CoreServices/powerd.bundle"),
                                                           timeout,
                                                           kIOPMAssertionTimeoutActionRelease,
                                                           &id);
    if (result != kIOReturnSuccess) {
        return;
    }
    ::IOPMAssertionRelease(id);
}
#endif

class DeviceFactory::DeviceStateNotificationImpl {
public:
	explicit DeviceStateNotificationImpl(std::weak_ptr<DeviceStateListener> callback) {
#ifdef XAMP_OS_WIN
		notification = new win32::Win32DeviceStateNotification(callback);
#else
        notification = MakeAlign<osx::CoreAudioDeviceStateNotification>(callback);
        AwakeFromDisplaySleep();
#endif
	}
#ifdef XAMP_OS_WIN
	CComPtr<win32::Win32DeviceStateNotification> notification;
#else
	align_ptr<osx::CoreAudioDeviceStateNotification> notification;
#endif
};

#define XAMP_REGISTER_DEVICE_TYPE(DeviceTypeClass) \
	creator_.emplace(DeviceTypeClass::Id, []() {\
		return MakeAlign<DeviceType, DeviceTypeClass>();\
	})

DeviceFactory::DeviceFactory() {
#ifdef XAMP_OS_WIN
	using namespace win32;	
	HrIfFailledThrow(::MFStartup(MF_VERSION, MFSTARTUP_LITE));
	XAMP_REGISTER_DEVICE_TYPE(SharedWasapiDeviceType);
	XAMP_REGISTER_DEVICE_TYPE(ExclusiveWasapiDeviceType);
#if ENABLE_ASIO
	XAMP_REGISTER_DEVICE_TYPE(ASIODeviceType);
#endif
#else
	using namespace osx;
    SetRealtimeProcessPriority();
    XAMP_REGISTER_DEVICE_TYPE(CoreAudioDeviceType);
#endif
}

DeviceFactory::~DeviceFactory() {
#ifdef XAMP_OS_WIN
	::MFShutdown();
#endif
}

void DeviceFactory::Clear() {
	creator_.clear();
}

std::optional<align_ptr<DeviceType>> DeviceFactory::CreateDefaultDevice() const {
	auto itr = creator_.begin();
	if (itr == creator_.end()) {
		return std::nullopt;
	}
	return (*itr).second();
}

std::optional<align_ptr<DeviceType>> DeviceFactory::Create(const ID id) const {
	auto itr = creator_.find(id);
	if (itr == creator_.end()) {
		return std::nullopt;
	}
	return (*itr).second();
}

bool DeviceFactory::IsPlatformSupportedASIO() const {
#if ENABLE_ASIO && defined(XAMP_OS_WIN)
	return creator_.find(ASIODeviceType::Id) != creator_.end();
#else
	return false;
#endif
}

bool DeviceFactory::IsExclusiveDevice(const DeviceInfo& info) {
#ifdef XAMP_OS_WIN
	return info.device_type_id == win32::ExclusiveWasapiDeviceType::Id
#if ENABLE_ASIO
		|| info.device_type_id == ASIODeviceType::Id
#endif
	;
#else
    (void)info;
    return false;
#endif
}

bool DeviceFactory::IsASIODevice(const ID id) {
#ifdef XAMP_OS_WIN
	return id == ASIODeviceType::Id;
#else
    (void)id;
	return false;
#endif
}

bool DeviceFactory::IsDeviceTypeExist(const ID id) const {
	return creator_.find(id) != creator_.end();
}

void DeviceFactory::RegisterDeviceListener(std::weak_ptr<DeviceStateListener> callback) {	
	impl_ = MakeAlign<DeviceStateNotificationImpl>(callback);
	impl_->notification->Run();
}

}
