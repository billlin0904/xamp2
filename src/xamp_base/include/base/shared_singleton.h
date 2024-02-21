//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <typeindex>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

/*
* GetSharedInstance is a function that can be called by different modules to get a shared singleton instance.
* 
* @param[in] type_index The type index of the singleton instance.
* @param[in] get_static_instance A function that returns the static instance of the singleton.
* @param[out] instance The shared singleton instance.
* 
*/
XAMP_BASE_API void GetSharedInstance(const std::type_index& type_index,
	void* (*get_static_instance)(),
	void*& instance);

/*
* SharedSingleton is a singleton class that can be shared between different modules.
* 
* The singleton instance is created by the first module that calls GetInstance().
* The singleton instance is destroyed when the last module that calls GetInstance() is unloaded.
* 
* @param T The type of the singleton instance.
* 
*/
template <typename T>
class XAMP_BASE_API_ONLY_EXPORT SharedSingleton {
public:
	static T& GetInstance() {
		static void* instance = nullptr;
        if (!instance) {
			GetSharedInstance(typeid(T), &GetStaticInstance, instance);
		}
		return *static_cast<T*>(instance);
	}

protected:
	/*
	* GetStaticInstance is a function that returns the static instance of the singleton.
	* 
	* @return The static instance of the singleton.
	*/
	static void* GetStaticInstance() {
		static T t{};
		return &reinterpret_cast<char&>(t);
	}

public:
	SharedSingleton() noexcept = delete;
	~SharedSingleton() noexcept = delete;

	XAMP_DISABLE_COPY(SharedSingleton)
};

XAMP_BASE_NAMESPACE_END

