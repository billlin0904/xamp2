#include <base/base.h>
#include <base/logger.h>

#ifdef XAMP_OS_WIN
#include <base/platform.h>
#include <output_device/win32/hrexception.h>
#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/win32/sharedwasapidevicetype.h>
#include <output_device/win32/win32devicestatenotification.h>
#if ENABLE_ASIO
#include <output_device/win32/mmcss.h>
#include <output_device/asiodevicetype.h>
#include <output_device/asiodevice.h>
#endif
#else
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <output_device/osx/coreaudiodevicetype.h>
#include <output_device/osx/hogcoreaudiodevicetype.h>
#include <output_device/osx/coreaudiodevicestatenotification.h>
#endif

#include <output_device/audiodevicemanager.h>

namespace xamp::output_device {

class AudioDeviceManager::DeviceStateNotificationImpl {
public:
#ifdef XAMP_OS_WIN
    using DeviceStateNotification = win32::Win32DeviceStateNotification;
    using DeviceStateNotificationPtr = CComPtr<DeviceStateNotification>;
#else
    using DeviceStateNotification = osx::CoreAudioDeviceStateNotification;
    using DeviceStateNotificationPtr = AlignPtr<DeviceStateNotification>;
#endif

    explicit DeviceStateNotificationImpl(std::weak_ptr<DeviceStateListener> const& callback) {
#ifdef XAMP_OS_WIN
        notification_ = new DeviceStateNotification(callback);
#else
        notification_.reset(new DeviceStateNotification(callback));
#endif
    }

    void Run() const {
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
	XAMP_LOG_DEBUG("Register {} success", #DeviceTypeClass); \
	factory_.emplace(DeviceTypeClass::Id, []() {\
		return MakeAlign<DeviceType, DeviceTypeClass>();\
	})

AudioDeviceManager::AudioDeviceManager() {
#ifdef XAMP_OS_WIN
    using namespace win32;
    HrIfFailledThrow(::MFStartup(MF_VERSION, MFSTARTUP_LITE));
    XAMP_LOG_DEBUG("MFStartup startup success");
#if ENABLE_ASIO
    Mmcss::LoadAvrtLib();
    XAMP_LOG_DEBUG("LoadAvrtLib success");
    XAMP_REGISTER_DEVICE_TYPE(ASIODeviceType);    
#endif
    XAMP_REGISTER_DEVICE_TYPE(SharedWasapiDeviceType);
    XAMP_REGISTER_DEVICE_TYPE(ExclusiveWasapiDeviceType);
#else
    using namespace osx;
    XAMP_REGISTER_DEVICE_TYPE(CoreAudioDeviceType);
#endif
}

void AudioDeviceManager::SetWorkingSetSize(size_t workingset_size) {
#ifdef XAMP_OS_WIN
    InitWorkingSetSize(workingset_size);
#endif
}

AudioDeviceManager::~AudioDeviceManager() {
#ifdef XAMP_OS_WIN	
    ::MFShutdown();
#else
    iopmAssertion.Reset();
#endif
}

void AudioDeviceManager::Clear() {
    factory_.clear();
}

AlignPtr<DeviceType> AudioDeviceManager::CreateDefaultDeviceType() const {
#ifdef XAMP_OS_WIN
    return Create(win32::SharedWasapiDeviceType::Id);
#else
    return Create(osx::CoreAudioDeviceType::Id);
#endif
}

AlignPtr<DeviceType> AudioDeviceManager::Create(Uuid const& id) const {
    auto itr = factory_.find(id);
    if (itr == factory_.end()) {
        throw DeviceNotFoundException();
    }
    return (*itr).second();
}

bool AudioDeviceManager::IsSupportASIO() const noexcept {
#if ENABLE_ASIO && defined(XAMP_OS_WIN)
    return IsDeviceTypeExist(ASIODeviceType::Id);
#else
    return false;
#endif
}

std::vector<Uuid> AudioDeviceManager::GetAvailableDeviceType() const {
    std::vector<Uuid> device_types;
	for (auto [uuid, device_type] : factory_) {
        device_types.push_back(uuid);
	}
    return device_types;
}

bool AudioDeviceManager::IsExclusiveDevice(DeviceInfo const & info) noexcept {
#ifdef XAMP_OS_WIN
    Uuid const device_type_id(info.device_type_id);
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

bool AudioDeviceManager::IsASIODevice(Uuid const& id) noexcept {
#if defined(ENABLE_ASIO) && defined(XAMP_OS_WIN)
    return id == ASIODeviceType::Id;
#else
    (void)id;
    return false;
#endif
}

void AudioDeviceManager::RemoveASIODriver() {
#if defined(ENABLE_ASIO) && defined(XAMP_OS_WIN)
    AsioDevice::RemoveDriver();
#endif
}

bool AudioDeviceManager::IsDeviceTypeExist(Uuid const& id) const noexcept {
    return factory_.find(id) != factory_.end();
}

void AudioDeviceManager::RegisterDeviceListener(std::weak_ptr<DeviceStateListener> const& callback) {
    impl_ = MakeAlign<DeviceStateNotificationImpl>(callback);
    impl_->Run();
}

void AudioDeviceManager::PreventSleep(bool allow) {
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
