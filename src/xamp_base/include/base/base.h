//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>

#define XAMP_CPP20_LANG_VER 202002L

#ifdef _WIN32
	#ifdef BASE_API_EXPORTS
		#define XAMP_BASE_API __declspec(dllexport)
	#else
		#define XAMP_BASE_API __declspec(dllimport)
	#endif
	#pragma warning(disable: 4251)
	#pragma warning(disable: 4275)
	#define XAMP_OS_WIN 1	
#else
    #define XAMP_BASE_API __attribute__((visibility("default")))
	#define XAMP_OS_MAC 1
#endif

#define XAMP_DISABLE_COPY(Class) \
	Class(const Class &) = delete; \
	Class& operator=(const Class &) = delete;

#define XAMP_DISABLE_MOVE(Class)\
    Class(Class&&) = delete;\
    Class& operator=(Class&&) = delete

#define XAMP_DISABLE_COPY_AND_MOVE(Class) \
	XAMP_DISABLE_COPY(Class);\
	XAMP_DISABLE_MOVE(Class);\

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
    Class(Class &&) ; \
	Class& operator=(Class &&) ;

#define XAMP_PIMPL_IMPL(Class) \
    Class::~Class() = default; \
    Class::Class(Class &&) = default; \
    Class& Class::operator=(Class &&) = default;

#ifdef XAMP_OS_WIN
#define XAMP_NO_VTABLE __declspec(novtable)
#else
#define XAMP_NO_VTABLE
#endif

#ifdef XAMP_OS_WIN
// Optimization function call
#define XAMP_ALWAYS_INLINE inline __forceinline
#define XAMP_NEVER_INLINE __declspec(noinline)
#else
#define XAMP_ALWAYS_INLINE inline __attribute__((__always_inline__))
#define XAMP_NEVER_INLINE __attribute__((__noinline__))
#endif

#if _MSVC_LANG >= XAMP_CPP20_LANG_VER
#define XAMP_CACHE_ALIGNED(CacheLineSize) alignas(CacheLineSize)
#else
#ifdef XAMP_OS_WIN
#define XAMP_CACHE_ALIGNED(CacheLineSize) __declspec(align(CacheLineSize))
#else
#define XAMP_CACHE_ALIGNED(CacheLineSize) __attribute__((aligned(CacheLineSize)))
#endif
#endif

#ifdef XAMP_OS_WIN
#define XAMP_NO_TLS_GUARDS [[msvc::no_tls_guard]]
#else
#define XAMP_NO_TLS_GUARDS
#endif

#ifdef XAMP_OS_WIN
#define XAMP_CHECK_LIFETIME [[msvc::lifetimebound]]
#else
#define XAMP_CHECK_LIFETIME
#endif

#define XAMP_HTTP_USER_AGENT "xamp2/1.0.0"

#define XAMP_BASE_NAMESPACE_BEGIN namespace xamp { namespace base {
#define XAMP_BASE_NAMESPACE_END } }

XAMP_BASE_NAMESPACE_BEGIN

/*
* Avoid cache-pollution padding size.
*/
inline constexpr size_t kCacheAlignSize = 64;

/*
* Memory allocate aligned size
* Assume we need 32-byte alignment for AVX2 instructions.
*/
inline constexpr size_t kMallocAlignSize{ 32 };

XAMP_BASE_NAMESPACE_END
