//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <filesystem>

#include <new>

#ifdef _WIN32
	#ifdef BASE_API_EXPORTS
		#define XAMP_BASE_API __declspec(dllexport)
		#define XAMP_BASE_API_ONLY_EXPORT __declspec(dllexport)
	#else
		#define XAMP_BASE_API __declspec(dllimport)
		#define XAMP_BASE_API_ONLY_EXPORT
	#endif
#if _MSVC_LANG > 201704L // Only for C++20
	#define XAMP_LIKELY(x) if(x) [[likely]]
	#define XAMP_UNLIKELY(x) if (!x) [[unlikely]]	
#else
	#define XAMP_LIKELY(x) if(x)
	#define XAMP_UNLIKELY(x) if (!x)
#endif
	#pragma warning(disable: 4251)
	#pragma warning(disable: 4275)
	#define XAMP_OS_WIN 1
	#define WIN32_LEAN_AND_MEAN
	#define XAMP_ENABLE_REP_MOVSB
#else
	#define XAMP_BASE_API
	#define XAMP_BASE_API_ONLY_EXPORT
	#define XAMP_LIKELY(x) if (__builtin_expect(!!(x), 1))
	#define XAMP_UNLIKELY(x) if (__builtin_expect(!!(x), 0))
	#define XAMP_OS_MAC 1
#endif

#define XAMP_DISABLE_COPY(Class) \
	Class(const Class &) = delete; \
	Class& operator=(const Class &) = delete;

#define XAMP_COMBIN(x, y) x##y
#define XAMP_COMBIN_NAME(x, y) XAMP_COMBIN(x, y)
#define XAMP_ANON_VAR_NAME(x) XAMP_COMBIN_NAME(x, __COUNTER__)

// Rule of five
// See more: http://en.cppreference.com/w/cpp/language/rule_of_three
#define XAMP_BASE_CLASS(Class) \
    virtual ~Class() = default; \
    Class(const Class &) = default; \
	Class& operator=(const Class &) = default; \
    Class(Class &&) = default; \
	Class& operator=(Class &&) = default;

#define XAMP_BASE_DISABLE_COPY_AND_MOVE(Class) \
    virtual ~Class() = default; \
    Class(const Class &) = delete; \
	Class& operator=(const Class &) = delete; \
    Class(Class &&) = delete; \
	Class& operator=(Class &&) = delete;

// Rule of five
// See more: http://en.cppreference.com/w/cpp/language/rule_of_three
#define XAMP_BASE_CLASS(Class) \
    virtual ~Class() = default; \
    Class(const Class &) = default; \
	Class& operator=(const Class &) = default; \
    Class(Class &&) = default; \
	Class& operator=(Class &&) = default;

// Scott Meyers C++11 PIMPL
// See more: http://oliora.github.io/2015/12/29/pimpl-and-rule-of-zero.html
#define XAMP_PIMPL(Class) \
    virtual ~Class(); \
    Class(const Class &) = delete; \
	Class& operator=(const Class &) = delete; \
    Class(Class &&) noexcept; \
	Class& operator=(Class &&) noexcept;

#define XAMP_PIMPL_IMPL(Class) \
    Class::~Class() = default; \
    Class::Class(Class &&) noexcept = default; \
    Class& Class::operator=(Class &&) noexcept = default;

#ifdef _WIN32
#define XAMP_NO_VTABLE __declspec(novtable)
#define XAMP_RESTRICT __restrict
#else
#define XAMP_NO_VTABLE
#define XAMP_RESTRICT __restrict__
#endif

#ifdef _WIN32
// Optimization function call
#define XAMP_ALWAYS_INLINE __forceinline
#define XAMP_NEVER_INLINE __declspec(noinline)
#else
#define XAMP_ALWAYS_INLINE inline __attribute__((__always_inline__))
#define XAMP_NEVER_INLINE __attribute__((__noinline__))
#endif

#ifdef _WIN32
#define XAMP_ALIGN_ASSUME_ALIGNED(ptr, alignment) ptr
#else
#define XAMP_ALIGN_ASSUME_ALIGNED(ptr, alignment) __builtin_assume_aligned(ptr, alignment)
#endif

#ifdef _WIN32
#ifndef NDEBUG
# define XAMP_NO_DEFAULT __assume(0)
#else
# define XAMP_NO_DEFAULT assert(0)
#endif
#else
# define XAMP_NO_DEFAULT assert(0)
#endif

#ifdef _WIN32
#define XAMP_CACHE_ALIGNED(CacheLineSize) __declspec(align(CacheLineSize))
#else
#define XAMP_CACHE_ALIGNED(CacheLineSize) __attribute__((aligned(CacheLineSize)))
#endif

#define XAMP_ENFORCE_TRIVIAL(t) \
static_assert(std::is_standard_layout_v<t>);\
static_assert(std::is_trivially_copyable_v<t>);\
static_assert(std::is_trivially_copy_assignable_v<t>);\
static_assert(std::is_trivially_copy_constructible_v<t>);\
static_assert(std::is_trivially_move_assignable_v<t>);\
static_assert(std::is_trivially_move_constructible_v<t>);\
static_assert(std::is_trivially_destructible_v<t>);\

#define XAMP_TRIVIAL_STRUCT(Name, ...)\
struct Name __VA_ARGS__;\
XAMP_ENFORCE_TRIVIAL(Name);

namespace xamp::base {
// <summary>
// Avoid cache-pollution padding size.
// </summary>
inline constexpr size_t kCacheAlignSize{ 64 };

/// <summary>
/// Memory allocate aligned size
/// Assume we need 32-byte alignment for AVX2 instructions.
/// </summary>
inline constexpr size_t kMallocAlignSize{ 32 };

/// <summary>
/// Default thread pool affinity core.
/// </summary>
inline constexpr int32_t kDefaultAffinityCpuCore{ -1 };

namespace Fs = std::filesystem;
using RecursiveDirectoryIterator = Fs::recursive_directory_iterator;
using DirectoryIterator = Fs::directory_iterator;
using Path = Fs::path;

inline constexpr auto kIteratorOptions = (
	std::filesystem::directory_options::follow_directory_symlink |
	std::filesystem::directory_options::skip_permission_denied
	);

constexpr double XAMP_PI = 3.14159265358979323846;

}
