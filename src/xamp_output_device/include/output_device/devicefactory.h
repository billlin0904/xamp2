//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <unordered_map>
#include <functional>
#include <optional>
#include <memory>

#include <base/base.h>
#include <base/stl.h>
#include <base/id.h>
#include <base/exception.h>
#include <base/align_ptr.h>
#include <base/logger.h>
#include <base/singleton.h>

#include <output_device/output_device.h>
#include <output_device/device_type.h>

namespace xamp::output_device {

using namespace base;

class XAMP_OUTPUT_DEVICE_API DeviceManager final {
public:
    ~DeviceManager();

    XAMP_DISABLE_COPY(DeviceManager)

    void RegisterDeviceListener(std::weak_ptr<DeviceStateListener> callback);

    void Clear();

    [[nodiscard]] std::optional<AlignPtr<DeviceType>> CreateDefaultDevice() const;

    [[nodiscard]] std::optional<AlignPtr<DeviceType>> Create(ID const& id) const;

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

    bool IsDeviceTypeExist(ID const& id) const noexcept;

    static DeviceManager& Default();

    static bool IsExclusiveDevice(DeviceInfo const &info) noexcept;

    static bool IsASIODevice(ID const& id) noexcept;

    static void RemoveASIOCurrentDriver();

    static void PreventSleep(bool allow);
private:
    class DeviceStateNotificationImpl;

    DeviceManager();

    template <typename Function>
    void RegisterCreator(ID const &id, Function&& fun) {
        factory_[id] = std::forward<Function>(fun);
    }

    AlignPtr<DeviceStateNotificationImpl> impl_;    
    HashMap<ID, std::function<AlignPtr<DeviceType>()>> factory_;
};

}

