//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_MAC

#include <functional>

#include <CoreAudio/AudioHardware.h>

#include <base/align_ptr.h>
#include <output_device/devicestatelistener.h>
#include <output_device/devicestatenotification.h>

namespace xamp::output_device::osx {

using namespace base;

class CoreAudioDeviceStateNotification : public DeviceStateNotification {
public:
    explicit CoreAudioDeviceStateNotification(std::weak_ptr<DeviceStateListener> callback);

    ~CoreAudioDeviceStateNotification() override;

    void Run() override;

private:
    void AddPropertyListener();

    void RemovePropertyListener();

    static OSStatus OnDefaultDeviceChangedCallback(
        AudioObjectID object,
        UInt32 num_addresses,
        AudioObjectPropertyAddress const addresses[],
        void* context);

    std::weak_ptr<DeviceStateListener> callback_;
};

}

#endif
