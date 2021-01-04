//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <unordered_map>
#include <functional>
#include <optional>
#include <memory>

#include <base/base.h>
#include <base/stl.h>
#include <base/uuid.h>
#include <base/exception.h>
#include <base/align_ptr.h>
#include <base/logger.h>

#include <output_device/output_device.h>
#include <output_device/device_type.h>

namespace xamp::output_device {

using namespace base;

class XAMP_OUTPUT_DEVICE_API AudioDeviceManager final {
public:
    ~AudioDeviceManager();

    XAMP_DISABLE_COPY(AudioDeviceManager)

    void RegisterDeviceListener(std::weak_ptr<DeviceStateListener> callback);

    void Clear();

    [[nodiscard]] std::optional<AlignPtr<DeviceType>> CreateDefaultDeviceType() const;

    [[nodiscard]] std::optional<AlignPtr<DeviceType>> Create(Uuid const& id) const;

    template <typename Function>
    void ForEach(Function &&fun) {
    	std::for_each(factory_.begin(), factory_.end(), [fun](auto const& creator) {
    		try {
                fun(creator.second());
            }
            catch (Exception const& e) {
                XAMP_LOG_DEBUG("{}", e.GetErrorMessage());
            }
    	});        
    }

    bool IsSupportASIO() const noexcept;

    bool IsDeviceTypeExist(Uuid const& id) const noexcept;

    static AudioDeviceManager& GetInstance();

    bool IsExclusiveDevice(DeviceInfo const &info) noexcept;

    bool IsASIODevice(Uuid const& id) noexcept;

    void RemoveASIODriver();

    void PreventSleep(bool allow);
private:
    class DeviceStateNotificationImpl;

    AudioDeviceManager();

    template <typename Function>
    void RegisterCreator(Uuid const &id, Function&& fun) {
        factory_[id] = std::forward<Function>(fun);
    }

    AlignPtr<DeviceStateNotificationImpl> impl_;    
    HashMap<Uuid, std::function<AlignPtr<DeviceType>()>> factory_;
};

}

