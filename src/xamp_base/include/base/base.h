//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cassert>
#include <cstdint>

#ifdef _WIN32
#ifdef BASE_API_EXPORTS
    #define XAMP_BASE_API __declspec(dllexport)
    #define XAMP_BASE_API_ONLY_EXPORT __declspec(dllexport)
#else
    #define XAMP_BASE_API __declspec(dllimport)
    #define XAMP_BASE_API_ONLY_EXPORT
#endif
#else
#define XAMP_BASE_API
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
#define XAMP_RESTRICT __declspec(restrict)
#else
#define XAMP_NO_VTABLE
#define XAMP_RESTRICT
#endif

#ifdef _WIN32
// Optimization function call
#define XAMP_ALWAYS_INLINE __forceinline
#define XAMP_NEVER_INLINE __declspec(noinline)
#else
#define XAMP_ALWAYS_INLINE inline
#define XAMP_NEVER_INLINE
#endif

#ifdef _WIN32
#ifdef DEBUG
# define XAMP_NO_DEFAULT assert(0)
#else
# define XAMP_NO_DEFAULT __assume(0)
#endif
#else
# define XAMP_NO_DEFAULT assert(0)
#endif

// Avoid cache-pollution padding size
#define XAMP_CACHE_ALIGN_SIZE 64

// Memory allocate aligned size
// Assume we need 32-byte alignment for AVX instructions.
#define XAMP_MALLOC_ALGIGN_SIZE 32

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
