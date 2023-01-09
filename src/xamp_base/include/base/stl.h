//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <vector>
#include <list>
#include <iterator>
#include <forward_list>
#include <functional>
#include <map>

#include <base/base.h>
#include <base/align_ptr.h>
#include <robin_hood.h>

namespace xamp::base {

#ifdef XAMP_OS_MAC
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
#endif

#ifdef XAMP_OS_MAC
#if __cplusplus >= XAMP_CPP20_LANG_VER
using std::bind_front;
#endif
#else
using std::bind_front;
#endif

template <typename T, size_t N>
constexpr size_t CountOf(T const (&)[N]) noexcept {
	return N;
}

template <typename T, size_t AlignmentBytes = kMallocAlignSize>
class XAMP_BASE_API_ONLY_EXPORT AlignedAllocator : public std::allocator<T> {
public:
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;

	template <typename U>
	struct rebind {
		typedef AlignedAllocator<U> other;
	};

	AlignedAllocator()
		: std::allocator<T>() {
	}

	AlignedAllocator(const AlignedAllocator& other)
		: std::allocator<T>(other) {
	}

	template <typename U>
	explicit AlignedAllocator(const AlignedAllocator<U>& other)
		: std::allocator<T>(other) {
	}

	~AlignedAllocator() {
	}

	template <typename C, typename... Args>
	void construct(C* c, Args&&... args) {
		new (reinterpret_cast<void*>(c)) C(std::forward<Args>(args)...);
	}

	template <typename U, typename V>
	void construct(U* ptr, V&& value) {
		::new(reinterpret_cast<void*>(ptr)) U(std::forward<V>(value));
	}

	template <typename U>
	void construct(U* ptr) {
		::new(reinterpret_cast<void*>(ptr)) U();
	}

	pointer allocate(size_type num, const void* /*hint*/ = nullptr) {
		return static_cast<pointer>(AlignedMalloc(num * sizeof(T), AlignmentBytes));
	}

	void deallocate(pointer p, size_type /*num*/) {
		AlignedFree(p);
	}
};

template <typename Type>
using Vector = std::vector<Type, AlignedAllocator<Type>>;

template <typename Type>
using ForwardList = std::forward_list<Type, AlignedAllocator<Type>>;

template <typename Type>
using List = std::list<Type, AlignedAllocator<Type>>;

template <typename T, typename... Args>
XAMP_BASE_API_ONLY_EXPORT std::shared_ptr<T> MakeAlignedShared(Args&&... args) {
	return std::allocate_shared<T>(AlignedAllocator<std::remove_const_t<T>>(), std::forward<Args>(args)...);
}

template <typename TP>
time_t ToTime_t(TP tp) {
	using namespace std::chrono;
	auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
		+ system_clock::now());
	return system_clock::to_time_t(sctp);
}

template <typename Resolution = std::chrono::microseconds>
time_t GetTime_t() {
	return std::chrono::duration_cast<Resolution>(
		std::chrono::system_clock::now().time_since_epoch()).count();
}

template <typename T, typename C>
Vector<T> Union(C const &a, C const &b) {
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

template<template <typename...> typename TMap, typename TKey, typename TValue>
std::vector<TKey> Keys(const TMap<TKey, TValue> &map) {
	std::vector<TKey> keys;
	keys.reserve(map.size());
	std::transform(map.begin(),
		map.end(),
		std::back_inserter(keys),
		[](const auto& pair) { return pair.first; });
	return keys;
}

template <template <typename...> typename TMap, typename TKey, typename TValue>
std::vector<TValue> Values(const TMap<TKey, TValue> &map) {
	std::vector<TValue> keys;
	keys.reserve(map.size());
	std::transform(map.begin(),
		map.end(),
		std::back_inserter(keys),
		[](const auto& pair) { return pair.second; });
	return keys;
}

template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>>
using HashMap = robin_hood::unordered_map<K, V, H, E>;

template <typename T, typename H = std::hash<T>, typename E = std::equal_to<T>>
using HashSet = robin_hood::unordered_set<T, H, E>;

template <typename K, typename V, typename P = std::less<K>>
using OrderedMap = std::map<K, V, P>;

template <typename ForwardIt, typename T, typename Compare = std::less<>>
ForwardIt BinarySearch(ForwardIt first, ForwardIt last, const T& value, Compare comp = {}) {
	first = std::lower_bound(first, last, value, comp);
	return first != last && !comp(*first, value) ? first : last;
}

}

