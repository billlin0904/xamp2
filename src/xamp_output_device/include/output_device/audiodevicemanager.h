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

#include <output_device/output_device.h>
#include <output_device/device_type.h>

namespace xamp::output_device {

using namespace base;

class XAMP_OUTPUT_DEVICE_API AudioDeviceManager final {
public:
    AudioDeviceManager();
	
    ~AudioDeviceManager();

    XAMP_DISABLE_COPY(AudioDeviceManager)

    void SetWorkingSetSize(size_t workingset_size);

    void RegisterDeviceListener(std::weak_ptr<DeviceStateListener> const & callback);

    void Clear();

    [[nodiscard]] AlignPtr<DeviceType> CreateDefaultDeviceType() const;

    [[nodiscard]] AlignPtr<DeviceType> Create(Uuid const& id) const;

    template <typename Func>
    void ForEach(Func&& func) {
    	std::for_each(factory_.begin(), factory_.end(), [func](auto const& creator) {
            func(creator.second());
    	});
    }

    [[nodiscard]] std::vector<Uuid> GetAvailableDeviceType() const;

    [[nodiscard]] bool IsSupportASIO() const noexcept;

    [[nodiscard]] bool IsDeviceTypeExist(Uuid const& id) const noexcept;    

    static bool IsExclusiveDevice(DeviceInfo const &info) noexcept;

    static bool IsASIODevice(Uuid const& id) noexcept;

    static void ResetASIODriver();

    static void PreventSleep(bool allow);
private:
    class DeviceStateNotificationImpl;    

    template <typename Func>
    void RegisterCreator(Uuid const &id, Func&& func) {
        factory_[id] = std::forward<Func>(func);
    }

    bool sleep_is_granular{ false };
    AlignPtr<DeviceStateNotificationImpl> impl_;    
    HashMap<Uuid, std::function<AlignPtr<DeviceType>()>> factory_;
};

}

