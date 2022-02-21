//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <deque>
#include <iterator>
#include <functional>

#include <base/base.h>
#include <robin_hood.h>

namespace xamp::base {

#if __cplusplus < XAMP_CPP20_LANG_VER
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

template <typename T, typename C>
std::vector<T> Union(C const &a, C const &b) {
	std::vector<T> result;
	result.reserve(a.size() + b.size());
	std::set_union(a.begin(), a.end(),
		b.begin(), b.end(),
		std::back_inserter(result));
	return result;
}
	
template <typename C>
double Median(C const& v) {
	C temp;
	std::copy(v.begin(), v.end(), std::back_inserter(temp));
	auto n = temp.size();
	
	if (n % 2 == 0) {
		std::nth_element(temp.begin(),
			temp.begin() + n / 2,
			temp.end());
		
		std::nth_element(temp.begin(),
			temp.begin() + (n - 1) / 2,
			temp.end());

		return static_cast<double>(temp[(n - 1) / 2]
				+ temp[n / 2])
				/ 2.0;
	} else {
		std::nth_element(temp.begin(),
			temp.begin() + n / 2,
			temp.end());
		return static_cast<double>(temp[n / 2]);
	}
}

template <typename T>
T Max(const std::vector<T>& v) {
	return *std::max_element(std::begin(v), std::end(v));
}

template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>>
using HashMap = robin_hood::unordered_map<K, V, H, E>;

template <typename T, typename H = std::hash<T>, typename E = std::equal_to<T>>
using HashSet = robin_hood::unordered_set<T, H, E>;

template <typename ForwardIt, typename T, typename Compare = std::less<>>
ForwardIt BinarySearch(ForwardIt first, ForwardIt last, const T& value, Compare comp = {}) {
	first = std::lower_bound(first, last, value, comp);
	return first != last && !comp(*first, value) ? first : last;
}


}

