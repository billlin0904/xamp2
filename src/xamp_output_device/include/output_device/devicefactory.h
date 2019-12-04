//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
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
#include <base/function_ref.h>
#include <base/align_ptr.h>
#include <base/logger.h>

#ifdef _WIN32
#include <output_device/win32/devicestatenotification.h>
#else
#include <output_device/osx/devicestatenotification.h>
#endif

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

	std::optional<AlignPtr<DeviceType>> CreateDefaultDevice() const;

	std::optional<AlignPtr<DeviceType>> Create(const ID id) const;

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
	DeviceFactory();

#ifdef _WIN32
	CComPtr<win32::DeviceStateNotification> notification_;
#else
    AlignPtr<osx::DeviceStateNotification> notification_;
#endif

	std::unordered_map<ID, FunctionRef<AlignPtr<DeviceType>()>> creator_;
};

}

