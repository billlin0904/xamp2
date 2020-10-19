#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <output_device/win32/hrexception.h>
#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/win32/sharedwasapidevicetype.h>
#include <output_device/win32/win32devicestatenotification.h>
#if ENABLE_ASIO
#include <output_device/win32/mmcss.h>
#include <output_device/asiodevicetype.h>
#endif
#else
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <output_device/osx/coreaudiodevicetype.h>
#include <output_device/osx/hogcoreaudiodevicetype.h>
#include <output_device/osx/coreaudiodevicestatenotification.h>
#endif

#include <base/platform_thread.h>

#include <output_device/devicefactory.h>

namespace xamp::output_device {

class DeviceManager::DeviceStateNotificationImpl {
public:
#ifdef XAMP_OS_WIN
    using DeviceStateNotification = win32::Win32DeviceStateNotification;
    using DeviceStateNotificationPtr = CComPtr<DeviceStateNotification>;
#else
    using DeviceStateNotification = osx::CoreAudioDeviceStateNotification;
    using DeviceStateNotificationPtr = AlignPtr<DeviceStateNotification>;
#endif

    explicit DeviceStateNotificationImpl(std::weak_ptr<DeviceStateListener> callback) {
#ifdef XAMP_OS_WIN
        notification_ = new DeviceStateNotification(callback);
#else
        notification_.reset(new DeviceStateNotification(callback));
#endif
    }

    void Run() {
        notification_->Run();
    }

private:
    DeviceStateNotificationPtr notification_;
};

#ifdef XAMP_OS_MAC
static struct IopmAssertion {
    IopmAssertion()
        : assertion_id(0) {
    }

    ~IopmAssertion() {
        Reset();
    }

    void PreventSleep() {
        if (assertion_id != 0) {
            Reset();
        }
        CFTimeInterval timeout = 5;
        ::IOPMAssertionCreateWithDescription(kIOPMAssertionTypePreventUserIdleSystemSleep,
                                             CFSTR("XAMP"),
                                             CFSTR("XAMP"),
                                             CFSTR("Prevents display sleep during playback"),
                                             CFSTR("/System/Library/CoreServices/powerd.bundle"),
                                             timeout,
                                             kIOPMAssertionTimeoutActionRelease,
                                             &assertion_id);
    }

    void Reset() {
        if (assertion_id == 0) {
            return;
        }
        ::IOPMAssertionRelease(assertion_id);
        assertion_id = 0;
    }

    IOPMAssertionID assertion_id;
} iopmAssertion;
#endif

#define XAMP_REGISTER_DEVICE_TYPE(DeviceTypeClass) \
	factory_.emplace(DeviceTypeClass::Id, []() {\
		return MakeAlign<DeviceType, DeviceTypeClass>();\
	})

DeviceManager::DeviceManager() {
#ifdef XAMP_OS_WIN
    using namespace win32;
    HrIfFailledThrow(::MFStartup(MF_VERSION, MFSTARTUP_LITE));
#if ENABLE_ASIO
    Mmcss::LoadAvrtLib();
    XAMP_REGISTER_DEVICE_TYPE(ASIODeviceType);
#endif
    XAMP_REGISTER_DEVICE_TYPE(SharedWasapiDeviceType);
    XAMP_REGISTER_DEVICE_TYPE(ExclusiveWasapiDeviceType);
#else
    using namespace osx;
    SetRealtimeProcessPriority();
    XAMP_REGISTER_DEVICE_TYPE(CoreAudioDeviceType);
    XAMP_REGISTER_DEVICE_TYPE(HogCoreAudioDeviceType);
#endif
}

DeviceManager& DeviceManager::Default() {
    static DeviceManager manager;
    return manager;
}

DeviceManager::~DeviceManager() {
#ifdef XAMP_OS_WIN
    ::MFShutdown();
#else
    iopmAssertion.Reset();
#endif
}

void DeviceManager::Clear() {
    factory_.clear();
}

std::optional<AlignPtr<DeviceType>> DeviceManager::CreateDefaultDevice() const {
    auto itr = factory_.begin();
    if (itr == factory_.end()) {
        return std::nullopt;
    }
    return (*itr).second();
}

std::optional<AlignPtr<DeviceType>> DeviceManager::Create(ID const& id) const {
    auto itr = factory_.find(id);
    if (itr == factory_.end()) {
        return std::nullopt;
    }
    return (*itr).second();
}

bool DeviceManager::IsSupportASIO() const noexcept {
#if ENABLE_ASIO && defined(XAMP_OS_WIN)
    return IsDeviceTypeExist(ASIODeviceType::Id);
#else
    return false;
#endif
}

bool DeviceManager::IsExclusiveDevice(DeviceInfo const & info) noexcept {
#ifdef XAMP_OS_WIN
    ID const device_type_id(info.device_type_id);
    return device_type_id == win32::ExclusiveWasapiDeviceType::Id
#if ENABLE_ASIO
           || device_type_id == ASIODeviceType::Id
#endif
        ;
#else
    (void)info;
    return false;
#endif
}

bool DeviceManager::IsASIODevice(ID const& id) noexcept {
#if defined(ENABLE_ASIO) && defined(XAMP_OS_WIN)
    return id == ASIODeviceType::Id;
#else
    (void)id;
    return false;
#endif
}

bool DeviceManager::IsDeviceTypeExist(ID const& id) const noexcept {
    return factory_.find(id) != factory_.end();
}

void DeviceManager::RegisterDeviceListener(std::weak_ptr<DeviceStateListener> callback) {	
    impl_ = MakeAlign<DeviceStateNotificationImpl>(callback);
    impl_->Run();
}

void DeviceManager::PreventSleep(bool allow) {
#ifdef XAMP_OS_WIN
    if (allow) {
        ::SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
    } else {
        ::SetThreadExecutionState(ES_CONTINUOUS);
    }
#else
    if (allow) {
        iopmAssertion.PreventSleep();
    } else {
        iopmAssertion.Reset();
    }
#endif
}

}
