//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <typeindex>
#include <base/base.h>

namespace xamp::base {

XAMP_BASE_API void GetSharedInstance(const std::type_index& type_index,
	void* (*get_static_instance)(),
	void*& instance);

/**
 * \brief Shared static singleton class.
 * \tparam T input type.
 */
template <typename T>
class XAMP_BASE_API_ONLY_EXPORT SharedSingleton {
public:
	static T& GetInstance() {
		static void* instance = nullptr;
		XAMP_UNLIKELY(instance) {
			GetSharedInstance(typeid(T), &GetStaticInstance, instance);
		}
		return *static_cast<T*>(instance);
	}

protected:
	static void* GetStaticInstance() {
		static T t{};
		return &reinterpret_cast<char&>(t);
	}
public:
	SharedSingleton() noexcept = delete;
	~SharedSingleton() noexcept = delete;

	XAMP_DISABLE_COPY(SharedSingleton)
};

}

