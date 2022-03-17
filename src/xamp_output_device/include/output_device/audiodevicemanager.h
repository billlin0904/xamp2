//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>
#include <map>

#include <base/base.h>
#include <base/stl.h>
#include <base/uuid.h>
#include <base/align_ptr.h>

#include <output_device/iaudiodevicemanager.h>
#include <output_device/idevicetype.h>

namespace xamp::output_device {

class AudioDeviceManager final : public IAudioDeviceManager {
public:
    AudioDeviceManager();
	
    virtual ~AudioDeviceManager();

    XAMP_DISABLE_COPY(AudioDeviceManager)

    void RegisterDeviceListener(std::weak_ptr<IDeviceStateListener> const & callback) override;

    void RegisterDevice(Uuid const& id, std::function<AlignPtr<IDeviceType>()> func) override;

    void Clear() override;

    [[nodiscard]] AlignPtr<IDeviceType> CreateDefaultDeviceType() const override;

    [[nodiscard]] AlignPtr<IDeviceType> Create(Uuid const& id) const override;

    DeviceTypeFactoryMap::iterator Begin() override;

    DeviceTypeFactoryMap::iterator End() override;

    [[nodiscard]] std::vector<Uuid> GetAvailableDeviceType() const override;

    [[nodiscard]] bool IsSupportASIO() const noexcept;

    [[nodiscard]] bool IsDeviceTypeExist(Uuid const& id) const noexcept;    
private:
    class DeviceStateNotificationImpl;
    AlignPtr<DeviceStateNotificationImpl> impl_;    
    DeviceTypeFactoryMap factory_;
};

}

