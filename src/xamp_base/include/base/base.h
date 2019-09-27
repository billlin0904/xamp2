//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#pragma warning(disable:4251)
#pragma warning(disable:4275)

#include <cassert>
#include <cstdint>

#ifdef BASE_API_EXPORTS
    #define XAMP_BASE_API __declspec(dllexport)
    #define XAMP_BASE_API_ONLY_EXPORT __declspec(dllexport)
#else
    #define XAMP_BASE_API __declspec(dllimport)
    #define XAMP_BASE_API_ONLY_EXPORT
#endif

#define XAMP_DISABLE_COPY(Class) \
	Class(const Class &) = delete; \
	Class& operator=(const Class &) = delete;

// Rule of five
// See more: http://en.cppreference.com/w/cpp/language/rule_of_three
#define XAMP_BASE_CLASS(Class) \
    virtual ~Class() = default; \
    Class(const Class &) = default; \
	Class& operator=(const Class &) = default; \
    Class(Class &&) = default; \
	Class& operator=(Class &&) = default;

// Scott Meyers¡¦ C++11 PIMPL
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

#define XAMP_NO_VTABLE __declspec(novtable) 

#define XAMP_RESTRICT __declspec(restrict)

// Optimization function call
#define XAMP_ALWAYS_INLINE __forceinline
#define XAMP_NEVER_INLINE __declspec(noinline)

#ifdef DEBUG
# define XAMP_NO_DEFAULT assert(0)
#else
# define XAMP_NO_DEFAULT __assume(0)
#endif

// Avoid cache-pollution padding size
#define XAMP_CACHE_ALIGN_SIZE 64

// Memory allocate aligned size
// Assume we need 32-byte alignment for AVX instructions.
#define XAMP_MALLOC_ALGIGN_SIZE 32

#define XAMP_ENABLE_FAST_MEMCPY 1
#define XAMP_ENABLE_ASIO 1
