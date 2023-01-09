//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ostream>
#include <string_view>
#include <array>

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

inline constexpr auto kMaxEnumSize = 20;

// Dll export must be in namespace scope. so we put this in global.
#define XAMP_MAKE_ENUM(EnumName, ...) enum class EnumName { __VA_ARGS__ }; \
static constexpr const std::array<std::string_view, kMaxEnumSize> EnumName##_enum_names = {\
    ALL_ARGUMENTS_TO_STRING(__VA_ARGS__)\
};\
inline constexpr std::string_view EnumToString(EnumName value) noexcept {\
    return EnumName##_enum_names[static_cast<size_t>(value)];\
}\
inline std::ostream &operator<<(std::ostream &os, EnumName value) {\
    os << EnumToString(value);\
    return os;\
}\

