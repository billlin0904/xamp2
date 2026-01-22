//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <typeindex>
#include <string_view>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

namespace detail {
	constexpr std::string_view ExtractClassNameFromFunSig(std::string_view sig) {
		// MSVC 格式假設:
		// "static class std::basic_string_view<...> __cdecl xamp::base::Logger::GetSingletonName(void) noexcept"

		constexpr std::string_view kCdecl = " __cdecl ";
		constexpr std::string_view kFunc = "::GetSingletonName";

		const auto pos_cdecl = sig.find(kCdecl);
		const auto start = (pos_cdecl == std::string_view::npos)
			? 0
			: pos_cdecl + kCdecl.size();

		const auto end = sig.find(kFunc, start);

		// 簡易防呆：如果找不到，就回傳整串（你也可以改成 static_assert）
		if (end == std::string_view::npos || start >= end) {
			return sig;
		}
		return sig.substr(start, end - start);
	}
}

#define XAMP_DECLARE_SINGLETON_NAME() \
	static constexpr std::string_view GetSingletonName() noexcept {          \
        constexpr std::string_view sig  = __FUNCSIG__;                       \
        constexpr std::string_view name =                                    \
            detail::ExtractClassNameFromFunSig(sig);						 \
        return name;                                                         \
    }


/*
* GetSharedInstance is a function that can be called by different modules to get a shared singleton instance.
* 
* @param[in] type_index The type index of the singleton instance.
* @param[in] get_static_instance A function that returns the static instance of the singleton.
* @param[out] instance The shared singleton instance.
* 
*/
XAMP_BASE_API void GetSharedInstance(std::string_view type_name,
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
		GetSharedInstance(T::GetSingletonName(), &GetStaticInstance, instance);
		return *static_cast<T*>(instance);
	}

protected:
	/*
	* GetStaticInstance is a function that returns the static instance of the singleton.
	* 
	* @return The static instance of the singleton.
	*/
	static void* GetStaticInstance() {
		static_assert(std::is_default_constructible_v<T>,
			"SharedSingleton<T> requires default-constructible T.");
		static T t{};
		return &t;
	}

public:
	SharedSingleton() noexcept = delete;
	~SharedSingleton() noexcept = delete;

	XAMP_DISABLE_COPY(SharedSingleton)
};

XAMP_BASE_NAMESPACE_END

