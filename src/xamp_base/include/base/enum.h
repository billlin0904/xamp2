//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#include <ostream>
#include <string_view>

#define EXPAND_VISUAL_STUDIO_HELPER(x) x
#define ALL_ARGUMENTS_TO_STRING_HELPER(zero,                                                     \
                                       a1,                                                       \
                                       a2,                                                       \
                                       a3,                                                       \
                                       a4,                                                       \
                                       a5,                                                       \
                                       a6,                                                       \
                                       a7,                                                       \
                                       a8,                                                       \
                                       a9,                                                       \
                                       a10,                                                      \
                                       a11,                                                      \
                                       a12,                                                      \
                                       a13,                                                      \
                                       a14,                                                      \
                                       a15,                                                      \
                                       a16,                                                      \
                                       a17,                                                      \
                                       a18,                                                      \
                                       a19,                                                      \
                                       a20,                                                      \
                                       ...)                                                      \
#a1, #a2, #a3, #a4, #a5, #a6, #a7, #a8, #a9, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17, \
        #a18, #a19, #a20

#define ALL_ARGUMENTS_TO_STRING(...)                            \
    EXPAND_VISUAL_STUDIO_HELPER(ALL_ARGUMENTS_TO_STRING_HELPER( \
        0, __VA_ARGS__, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))

#define MAKE_ENUM(Name, ...) enum class Name : uint8_t { __VA_ARGS__ }; \
namespace {\
static constexpr std::string_view Name##_enum_names[] = {\
    ALL_ARGUMENTS_TO_STRING(__VA_ARGS__)\
};\
XAMP_ALWAYS_INLINE constexpr std::string_view EnumToString(Name value) noexcept {\
    return Name##_enum_names[static_cast<int>(value)];\
}\
XAMP_ALWAYS_INLINE std::ostream &operator<<(std::ostream &os, Name value) {\
    os << EnumToString(value);\
    return os;\
}\
}\

