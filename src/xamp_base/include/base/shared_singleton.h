//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <typeindex>
#include <string_view>
#include <type_traits>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

namespace detail {
	constexpr std::string_view trimTypePrefix(std::string_view name) {
		constexpr std::string_view kClassPrefix = "class ";
		constexpr std::string_view kStructPrefix = "struct ";
		constexpr std::string_view kEnumPrefix = "enum ";

		if (name.starts_with(kClassPrefix)) {
			return name.substr(kClassPrefix.size());
		}
		if (name.starts_with(kStructPrefix)) {
			return name.substr(kStructPrefix.size());
		}
		if (name.starts_with(kEnumPrefix)) {
			return name.substr(kEnumPrefix.size());
		}
		return name;
	}

	constexpr std::string_view extractClassNameFromFunSig(std::string_view sig) {
		// MSVC 格式假設:
		// "static class std::basic_string_view<...> __cdecl xamp::base::Logger::getSingletonName(void)"

		constexpr std::string_view kCdecl = " __cdecl ";
		constexpr std::string_view kFunc = "::getSingletonName";
		constexpr std::string_view kOldFunc = "::GetSingletonName";

		const auto pos_cdecl = sig.find(kCdecl);
		const auto start = (pos_cdecl == std::string_view::npos)
			? 0
			: pos_cdecl + kCdecl.size();

		auto end = sig.find(kFunc, start);
		if (end == std::string_view::npos) {
			end = sig.find(kOldFunc, start);
		}

		// 簡易防呆：如果找不到，就回傳整串（你也可以改成 static_assert）
		if (end == std::string_view::npos || start >= end) {
			return sig;
		}
		return trimTypePrefix(sig.substr(start, end - start));
	}

	constexpr std::string_view extractClassNameFromPrettyFunction(std::string_view sig) {
		constexpr std::string_view kPrefix = "static constexpr std::string_view ";
		constexpr std::string_view kFunc = "::getSingletonName";
		constexpr std::string_view kOldFunc = "::GetSingletonName";

		const auto prefix = sig.find(kPrefix);
		const auto start = prefix == std::string_view::npos ? 0 : prefix + kPrefix.size();
		auto end = sig.find(kFunc, start);
		if (end == std::string_view::npos) {
			end = sig.find(kOldFunc, start);
		}
		if (end == std::string_view::npos || start >= end) {
			return sig;
		}
		return trimTypePrefix(sig.substr(start, end - start));
	}

	template <typename T>
	constexpr std::string_view singletonTypeName() {
#ifdef _MSC_VER
		constexpr std::string_view sig = __FUNCSIG__;
		constexpr std::string_view kPrefix = "singletonTypeName<";
		const auto prefix = sig.find(kPrefix);
		const auto start = prefix == std::string_view::npos ? 0 : prefix + kPrefix.size();
		const auto end = sig.find(">(void)", start);
		if (end == std::string_view::npos || start >= end) {
			return sig;
		}
		return trimTypePrefix(sig.substr(start, end - start));
#else
		constexpr std::string_view sig = __PRETTY_FUNCTION__;
		constexpr std::string_view kPrefix = "T = ";
		const auto prefix = sig.find(kPrefix);
		const auto start = prefix == std::string_view::npos ? 0 : prefix + kPrefix.size();
		auto end = sig.find(';', start);
		if (end == std::string_view::npos) {
			end = sig.find(']', start);
		}
		if (end == std::string_view::npos || start >= end) {
			return sig;
		}
		return trimTypePrefix(sig.substr(start, end - start));
#endif
	}
}

#ifdef _MSC_VER
#define XAMP_DECLARE_SINGLETON_NAME() \
	static constexpr std::string_view getSingletonName() {          \
        constexpr std::string_view sig  = __FUNCSIG__;                       \
        constexpr std::string_view name =                                    \
            ::xamp::base::detail::extractClassNameFromFunSig(sig);			 \
        return name;                                                         \
    }
#else
#define XAMP_DECLARE_SINGLETON_NAME() \
	static constexpr std::string_view getSingletonName() {          \
        constexpr std::string_view sig  = __PRETTY_FUNCTION__;               \
        constexpr std::string_view name =                                    \
            ::xamp::base::detail::extractClassNameFromPrettyFunction(sig);	 \
        return name;                                                         \
    }
#endif

/*
* getSharedInstance is a function that can be called by different modules to get a shared singleton instance.
* 
* @param[in] type_index The type index of the singleton instance.
* @param[in] get_static_instance A function that returns the static instance of the singleton.
* @param[out] instance The shared singleton instance.
* 
*/
XAMP_BASE_API void getSharedInstance(std::string_view type_name,
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
class SharedSingleton {
public:
	static T& getInstance() {
		static void* instance = nullptr;
		getSharedInstance(detail::singletonTypeName<T>(), &getStaticInstance, instance);
		return *static_cast<T*>(instance);
	}

protected:
	/*
	* getStaticInstance is a function that returns the static instance of the singleton.
	* 
	* @return The static instance of the singleton.
	*/
	static void* getStaticInstance() {
		static_assert(std::is_default_constructible_v<T>,
			"SharedSingleton<T> requires default-constructible T.");
		static T t{};
		return &t;
	}

public:
	SharedSingleton() = delete;
	~SharedSingleton() = delete;

	XAMP_DISABLE_COPY(SharedSingleton)
};

XAMP_BASE_NAMESPACE_END
