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
#include <base/id.h>
#include <base/exception.h>
#include <base/align_ptr.h>
#include <base/logger.h>

#include <output_device/output_device.h>
#include <output_device/device_type.h>

namespace xamp::output_device {

using namespace base;

class XAMP_OUTPUT_DEVICE_API AudioDeviceFactory final {
public:	
    ~AudioDeviceFactory();

    XAMP_DISABLE_COPY(AudioDeviceFactory)

    static AudioDeviceFactory& Instance() {
        static AudioDeviceFactory factory;
        return factory;
    }

    void RegisterDeviceListener(std::weak_ptr<DeviceStateListener> callback);

    void Clear();

    std::optional<AlignPtr<DeviceType>> CreateDefaultDevice() const;

    std::optional<AlignPtr<DeviceType>> Create(ID const id) const;

    template <typename Function>
    void ForEach(Function &&fun) {
        for (const auto& creator : creator_) {
            try {
                fun(creator.second());
            }
            catch (const Exception& e) {
                XAMP_LOG_DEBUG("{}", e.GetErrorMessage());
            }
        }
    }

    bool IsPlatformSupportedASIO() const;

    bool IsDeviceTypeExist(ID const id) const;

    static bool IsExclusiveDevice(DeviceInfo const &info);

    static bool IsASIODevice(const ID id);

    static void PreventSleep(bool allow);
private:
    class DeviceStateNotificationImpl;

    AudioDeviceFactory();

    template <typename Function>
    void RegisterCreator(ID const id, Function&& fun) {
        creator_[id] = std::forward<Function>(fun);
    }

    AlignPtr<DeviceStateNotificationImpl> impl_;
    RobinHoodHashMap<ID, std::function<AlignPtr<DeviceType>()>> creator_;
};

}

