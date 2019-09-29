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

#include <output_device/output_device.h>
#include <output_device/device_type.h>

namespace xamp::output_device {

using namespace base;

class XAMP_OUTPUT_DEVICE_API DeviceFactory {
public:	
	~DeviceFactory();

	XAMP_DISABLE_COPY(DeviceFactory)

	static DeviceFactory& Instance() {
		static DeviceFactory factory;
		return factory;
	}

	template <typename Function>
	void RegisterCreator(const ID id, Function &&fun) {
		creator_[id] = std::move(fun);
	}

	void Clear() {
		creator_.clear();
	}

	std::optional<AlignPtr<DeviceType>> CreateDefaultDevice() const {
		auto itr = creator_.begin();
		if (itr == creator_.end()) {
			return std::nullopt;
		}
		return (*itr).second();
	}

	std::optional<AlignPtr<DeviceType>> Create(const ID id) const {
		auto itr = creator_.find(id);
		if (itr == creator_.end()) {
			return std::nullopt;
		}
		return (*itr).second();
	}

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

	static bool IsExclusiveDevice(const DeviceInfo &info);

private:
	DeviceFactory();

	std::unordered_map<ID, FunctionRef<AlignPtr<DeviceType>()>> creator_;
};

}

