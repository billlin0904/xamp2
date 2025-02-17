//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <vector>
#include <list>
#include <iterator>
#include <forward_list>
#include <functional>
#include <memory>
#include <map>

#include <base/base.h>
#include <base/memory.h>

//#define XAMP_USE_STD_MAP

#ifdef XAMP_USE_STD_MAP
#include <unordered_map>
#include <unordered_set>
#else
#include <ankerl/unordered_dense.h>
#endif

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_MAC
template <typename T, typename ...Args>
auto tuple_append(T&& t, Args&&...args) {
	return std::tuple_cat(
		std::forward<T>(t),
		std::forward_as_tuple(args...)
	);
}

template<typename F, typename ...FrontArgs>
decltype(auto) bind_front(F&& f, FrontArgs&&...front_args) {
	return[f = std::forward<F>(f),
		frontArgs = std::make_tuple(std::forward<FrontArgs>(front_args)...)]
		(auto&&...back_args) {
		return std::apply(
			f,
			tuple_append(
				frontArgs,
				std::forward<decltype(back_args)>(back_args)...));
	};
}
#endif

#ifdef XAMP_OS_WIN
using std::bind_front;
#endif

template <typename T, size_t N>
constexpr size_t CountOf(T const (&)[N]) noexcept {
	return N;
}

template <typename T, typename... Args>
std::optional<T> MakeOptional(Args&&... args) {
	return std::optional<T>(std::in_place_t{}, std::forward<Args>(args)...);
}

#ifdef XAMP_USE_STD_MAP
template <typename T1, typename T2>
using Pair = std::pair<T1, T2>;

template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename Alloc = AlignedAllocator<std::pair<const K, V>>>
using HashMap = std::unordered_map<K, V, H, E, Alloc>;

template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename Alloc = AlignedAllocator<std::pair<const K, V>>>
using FloatMap = std::unordered_map<K, V, H, E, Alloc>;

template <typename T, typename H = std::hash<T>, typename E = std::equal_to<T>, typename Alloc = AlignedAllocator<T>>
using HashSet = std::unordered_set<T, H, E, Alloc>;
#else

template <typename K, typename V>
using FloatMap = ankerl::unordered_dense::map<K, V>;

template <typename K, typename V>
using HashMap = ankerl::unordered_dense::map<K, V>;

template <typename K>
using HashSet = ankerl::unordered_dense::set<K>;
#endif

template <typename K, typename V, typename P = std::less<K>>
using OrderedMap = std::map<K, V, P>;

XAMP_BASE_NAMESPACE_END
