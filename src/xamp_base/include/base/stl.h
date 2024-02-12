//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <vector>
#include <list>
#include <iterator>
#include <forward_list>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <map>

#include <base/base.h>
#include <base/align_ptr.h>

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

	~AlignedAllocator() = default;

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

#define XAMP_USE_STD_MAP

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
template <typename T1, typename T2>
using Pair = robin_hood::pair<T1, T2>;

template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename Alloc = AlignedAllocator<std::pair<const K, V>>>
using FloatMap = robin_hood::unordered_flat_map<K, V, H, E>;

template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename Alloc = AlignedAllocator<std::pair<const K, V>>>
using HashMap = robin_hood::unordered_map<K, V, H, E>;

template <typename T, typename H = std::hash<T>, typename E = std::equal_to<T>, typename Alloc = AlignedAllocator<T>>
using HashSet = robin_hood::unordered_set<T, H, E>;
#endif

template <typename K, typename V, typename P = std::less<K>, typename Alloc = AlignedAllocator<std::pair<const K, V>>>
using OrderedMap = std::map<K, V, P, Alloc>;

XAMP_BASE_NAMESPACE_END
