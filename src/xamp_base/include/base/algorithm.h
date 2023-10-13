//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/stl.h>

XAMP_BASE_NAMESPACE_BEGIN

template <typename TP>
time_t ToTime_t(TP tp) {
	using namespace std::chrono;
	auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
		+ system_clock::now());
	return system_clock::to_time_t(sctp);
}

template <typename Resolution = std::chrono::seconds>
time_t GetTime_t() {
	return std::chrono::duration_cast<Resolution>(
		std::chrono::system_clock::now().time_since_epoch()).count();
}

template <typename T, typename C>
Vector<T> Union(C const& a, C const& b) {
	Vector<T> result;
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
	}
	else {
		std::nth_element(temp.begin(),
			temp.begin() + n / 2,
			temp.end());
		return static_cast<double>(temp[n / 2]);
	}
}

template <typename T>
T Max(const Vector<T>& v) {
	return *std::max_element(std::begin(v), std::end(v));
}

template<template <typename...> typename TMap, typename TKey, typename TValue>
Vector<TKey> Keys(const TMap<TKey, TValue>& map) {
	Vector<TKey> keys;
	keys.reserve(map.size());
	std::transform(map.begin(),
		map.end(),
		std::back_inserter(keys),
		[](const auto& pair) { return pair.first; });
	return keys;
}

template <template <typename...> typename TMap, typename TKey, typename TValue>
Vector<TValue> Values(const TMap<TKey, TValue>& map) {
	Vector<TValue> keys;
	keys.reserve(map.size());
	std::transform(map.begin(),
		map.end(),
		std::back_inserter(keys),
		[](const auto& pair) { return pair.second; });
	return keys;
}

template <typename ForwardIt, typename T, typename Compare = std::less<>>
ForwardIt BinarySearch(ForwardIt first, ForwardIt last, const T& value, Compare comp = {}) {
	first = std::lower_bound(first, last, value, comp);
	return first != last && !comp(*first, value) ? first : last;
}

XAMP_BASE_NAMESPACE_END

