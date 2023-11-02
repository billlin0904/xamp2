#include <output_device/audiodevicemanager.h>
#include <output_device/api.h>

#ifdef XAMP_OS_WIN
#include <mfapi.h>
#include <base/platform.h>
#include <output_device/win32/comexception.h>
#include <output_device/win32/exclusivewasapidevice.h>
#include <output_device/win32/exclusivewasapidevicetype.h>
#include <output_device/win32/sharedwasapidevicetype.h>
#include <output_device/win32/win32devicestatenotification.h>
#include <output_device/win32/nulloutputdevicetype.h>
#include <output_device/win32/mmcss.h>
#include <output_device/win32/asiodevice.h>
#include <output_device/win32/asiodevicetype.h>
#else
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <output_device/osx/osx_utitl.h>
#include <output_device/osx/coreaudiodevicetype.h>
#include <output_device/osx/hogcoreaudiodevicetype.h>
#include <output_device/osx/coreaudiodevicestatenotification.h>
#endif

#include <ranges>
#include <base/base.h>
#include <base/assert.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/exception.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

class AudioDeviceManager::DeviceStateNotificationImpl {
public:
#ifdef XAMP_OS_WIN
    using DeviceStateNotification = win32::Win32DeviceStateNotification;
    using DeviceStateNotificationPtr = CComPtr<DeviceStateNotification>;
#else
    using DeviceStateNotification = osx::CoreAudioDeviceStateNotification;
    using DeviceStateNotificationPtr = AlignPtr<DeviceStateNotification>;
#endif

    DeviceStateNotificationImpl() = default;

    void SetCallback(const std::weak_ptr<IDeviceStateListener> & callback) {
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

#define XAMP_REGISTER_DEVICE_TYPE(DeviceTypeClass) \
	XAMP_LOG_DEBUG("Register {} success", #DeviceTypeClass); \
    RegisterDevice(XAMP_UUID_OF(DeviceTypeClass), []() {\
		return MakeAlign<IDeviceType, DeviceTypeClass>();\
	})

AudioDeviceManager::AudioDeviceManager()
	: impl_(MakeAlign<DeviceStateNotificationImpl>()) {
#ifdef XAMP_OS_WIN
    using namespace win32;
    XAMP_LOG_DEBUG("LoadAvrtLib success");
    HrIfFailledThrow(::MFStartup(MF_VERSION, MFSTARTUP_LITE));
    XAMP_LOG_DEBUG("MFStartup startup success");
    XAMP_REGISTER_DEVICE_TYPE(SharedWasapiDeviceType);
    XAMP_REGISTER_DEVICE_TYPE(ExclusiveWasapiDeviceType);
    XAMP_REGISTER_DEVICE_TYPE(NullOutputDeviceType);
    XAMP_REGISTER_DEVICE_TYPE(AsioDeviceType);
#else
    using namespace osx;
    XAMP_REGISTER_DEVICE_TYPE(CoreAudioDeviceType);
    //XAMP_REGISTER_DEVICE_TYPE(HogCoreAudioDeviceType);
#endif
}

AudioDeviceManager::~AudioDeviceManager() {
#ifdef XAMP_OS_WIN	
    ::MFShutdown();    
#else
    PreventSleep(false);
#endif
}

void AudioDeviceManager::Clear() {
    factory_.clear();
}

AlignPtr<IDeviceType> AudioDeviceManager::CreateDefaultDeviceType() const {
#ifdef XAMP_OS_WIN
    return Create(XAMP_UUID_OF(win32::SharedWasapiDeviceType));
#else
    return Create(XAMP_UUID_OF(osx::CoreAudioDeviceType));
#endif
}

AlignPtr<IDeviceType> AudioDeviceManager::Create(const Uuid & id) const {
    auto itr = factory_.find(id);
    if (itr == factory_.end()) {
        throw DeviceNotFoundException();
    }
    return (*itr).second();
}

bool AudioDeviceManager::IsSupportAsio() const noexcept {
#if defined(XAMP_OS_WIN)
    return IsDeviceTypeExist(XAMP_UUID_OF(win32::AsioDeviceType));
#else
    return false;
#endif
}

DeviceTypeFactoryMap::iterator AudioDeviceManager::Begin() {
    return factory_.begin();
}

DeviceTypeFactoryMap::iterator AudioDeviceManager::End() {
    return factory_.end();
}

Vector<Uuid> AudioDeviceManager::GetAvailableDeviceType() const {
    Vector<Uuid> device_types;
    for (const auto& uuid : factory_ | std::views::keys) {
        device_types.push_back(uuid);
	}
    return device_types;
}

bool AudioDeviceManager::IsDeviceTypeExist(Uuid const& id) const noexcept {
    return factory_.contains(id);
}

void AudioDeviceManager::RegisterDeviceListener(std::weak_ptr<IDeviceStateListener> const& callback) {
    impl_->SetCallback(callback);
    impl_->Run();
}

void AudioDeviceManager::RegisterDevice(Uuid const& id, std::function<AlignPtr<IDeviceType>()> func) {
    factory_.emplace(std::make_pair(id, std::move(func)));
}

XAMP_OUTPUT_DEVICE_NAMESPACE_END
