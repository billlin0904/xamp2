//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <functional>
#include <memory>
#include <map>

#include <base/base.h>
#include <base/stl.h>
#include <base/uuid.h>
#include <base/exception.h>
#include <base/align_ptr.h>

#include <output_device/iaudiodevicemanager.h>
#include <output_device/output_device.h>
#include <output_device/idevicetype.h>

namespace xamp::output_device {

using namespace base;

class XAMP_OUTPUT_DEVICE_API AudioDeviceManager final : public IAudioDeviceManager {
public:
    AudioDeviceManager();
	
    virtual ~AudioDeviceManager();

    XAMP_DISABLE_COPY(AudioDeviceManager)

    void RegisterDeviceListener(std::weak_ptr<IDeviceStateListener> const & callback) override;

    void Clear() override;

    [[nodiscard]] AlignPtr<IDeviceType> CreateDefaultDeviceType() const override;

    [[nodiscard]] AlignPtr<IDeviceType> Create(Uuid const& id) const override;

    DeviceTypeFactoryMap::iterator Begin() override;

    DeviceTypeFactoryMap::iterator End() override;

    [[nodiscard]] std::vector<Uuid> GetAvailableDeviceType() const override;

    [[nodiscard]] bool IsSupportASIO() const noexcept;

    [[nodiscard]] bool IsDeviceTypeExist(Uuid const& id) const noexcept;

    static bool IsExclusiveDevice(DeviceInfo const &info) noexcept;

    static bool IsASIODevice(Uuid const& id) noexcept;

    static void ResetASIODriver();

    static void PreventSleep(bool allow);
private:
    class DeviceStateNotificationImpl;

    void SetWorkingSetSize(size_t workingset_size);

    template <typename Func>
    void RegisterCreator(Uuid const &id, Func&& func) {
        factory_[id] = std::forward<Func>(func);
    }

    bool sleep_is_granular_{ false };
    AlignPtr<DeviceStateNotificationImpl> impl_;    
    DeviceTypeFactoryMap factory_;
};

}

