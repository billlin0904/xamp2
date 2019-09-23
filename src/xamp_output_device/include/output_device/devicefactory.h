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
#include <output_device/output_device.h>
#include <output_device/device_type.h>

namespace xamp::output_device {

using namespace base;

#define XAMP_REGISTER_DEVICE_TYPE(DeviceTypeClass) \
	DeviceFactory::Instance().RegisterCreator(DeviceTypeClass::Id, []() {\
		return std::make_unique<DeviceTypeClass>();\
	})

class XAMP_OUTPUT_DEVICE_API DeviceFactory {
public:	
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

	std::optional<std::unique_ptr<DeviceType>> CreateDefaultDevice() const {
		auto itr = creator_.begin();
		if (itr == creator_.end()) {
			return std::nullopt;
		}
		return (*itr).second();
	}

	std::optional<std::unique_ptr<DeviceType>> Create(const ID id) const {
		auto itr = creator_.find(id);
		if (itr == creator_.end()) {
			return std::nullopt;
		}
		return (*itr).second();
	}

	template <typename Function>
	void ForEach(Function &&fun) {
		for (const auto& creator : creator_) {
			fun(creator.second());
		}
	}

	static bool IsExclusiveDevice(const DeviceInfo &info);

private:
	DeviceFactory() = default;
	std::unordered_map<ID, std::function<std::unique_ptr<DeviceType>()>> creator_;
};

}

