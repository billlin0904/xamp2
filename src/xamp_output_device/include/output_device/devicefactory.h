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

class XAMP_OUTPUT_DEVICE_API DeviceFactory final {
public:	
    ~DeviceFactory();

    XAMP_DISABLE_COPY(DeviceFactory)

    static DeviceFactory& Instance() {
        static DeviceFactory factory;
        return factory;
    }

    template <typename Function>
    void RegisterCreator(const ID id, Function &&fun) {
        creator_[id] = std::forward<Function>(fun);
    }

    void RegisterDeviceListener(std::weak_ptr<DeviceStateListener> callback);

    void Clear();

    std::optional<align_ptr<DeviceType>> CreateDefaultDevice() const;

    std::optional<align_ptr<DeviceType>> Create(const ID id) const;

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

    static bool IsExclusiveDevice(const DeviceInfo &info);

    static bool IsASIODevice(const ID id);

    bool IsDeviceTypeExist(const ID id) const;
private:
    class DeviceStateNotificationImpl;

    DeviceFactory();    

    align_ptr<DeviceStateNotificationImpl> impl_;
    RobinHoodHashMap<ID, std::function<align_ptr<DeviceType>()>> creator_;
};

}

