//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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
                                       a21,                                                      \
                                       a22,                                                      \
                                       a23,                                                      \
                                       a24,                                                      \
                                       a25,                                                      \
                                       a26,                                                      \
                                       a27,                                                      \
                                       a28,                                                      \
                                       a29,                                                      \
                                       a30,                                                      \
                                       a31,                                                      \
                                       a32,                                                      \
                                       a33,                                                      \
                                       a34,                                                      \
                                       a35,                                                      \
                                       a36,                                                      \
                                       a37,                                                      \
                                       a38,                                                      \
                                       a39,                                                      \
                                       a40,                                                      \
                                       a41,                                                      \
                                       a42,                                                      \
                                       a43,                                                      \
                                       a44,                                                      \
                                       a45,                                                      \
                                       a46,                                                      \
                                       a47,                                                      \
                                       a48,                                                      \
                                       a49,                                                      \
                                       a50,                                                      \
                                       a51,                                                      \
                                       a52,                                                      \
                                       a53,                                                      \
                                       a54,                                                      \
                                       a55,                                                      \
                                       a56,                                                      \
                                       a57,                                                      \
                                       a58,                                                      \
                                       a59,                                                      \
                                       a60,                                                      \
                                       a61,                                                      \
                                       a62,                                                      \
                                       a63,                                                      \
                                       a64,                                                      \
                                       a65,                                                      \
                                       a66,                                                      \
                                       a67,                                                      \
                                       a68,                                                      \
                                       a69,                                                      \
                                       a70,                                                      \
                                       a71,                                                      \
                                       a72,                                                      \
                                       a73,                                                      \
                                       a74,                                                      \
                                       a75,                                                      \
                                       a76,                                                      \
                                       a77,                                                      \
                                       a78,                                                      \
                                       a79,                                                      \
                                       a80,                                                      \
                                       a81,                                                      \
                                       a82,                                                      \
                                       a83,                                                      \
                                       a84,                                                      \
                                       a85,                                                      \
                                       a86,                                                      \
                                       a87,                                                      \
                                       a88,                                                      \
                                       a89,                                                      \
                                       a90,                                                      \
                                       a91,                                                      \
                                       a92,                                                      \
                                       a93,                                                      \
                                       a94,                                                      \
                                       a95,                                                      \
                                       a96,                                                      \
                                       a97,                                                      \
                                       a98,                                                      \
                                       a99,                                                      \
                                       a100,                                                     \
                                       ...)                                                      \
    #a1, #a2, #a3, #a4, #a5, #a6, #a7, #a8, #a9, #a10,                                           \
    #a11, #a12, #a13, #a14, #a15, #a16, #a17, #a18, #a19, #a20,                                  \
    #a21, #a22, #a23, #a24, #a25, #a26, #a27, #a28, #a29, #a30,                                  \
    #a31, #a32, #a33, #a34, #a35, #a36, #a37, #a38, #a39, #a40,                                  \
    #a41, #a42, #a43, #a44, #a45, #a46, #a47, #a48, #a49, #a50,                                  \
    #a51, #a52, #a53, #a54, #a55, #a56, #a57, #a58, #a59, #a60,                                  \
    #a61, #a62, #a63, #a64, #a65, #a66, #a67, #a68, #a69, #a70,                                  \
    #a71, #a72, #a73, #a74, #a75, #a76, #a77, #a78, #a79, #a80,                                  \
    #a81, #a82, #a83, #a84, #a85, #a86, #a87, #a88, #a89, #a90,                                  \
    #a91, #a92, #a93, #a94, #a95, #a96, #a97, #a98, #a99, #a100

#define ALL_ARGUMENTS_TO_STRING(...)                            \
    EXPAND_VISUAL_STUDIO_HELPER(ALL_ARGUMENTS_TO_STRING_HELPER( \
        0, __VA_ARGS__, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))

inline constexpr auto kMaxEnumSize = 100;

#define XAMP_MAKE_ENUM(EnumName, ...) enum class EnumName { __VA_ARGS__, COUNT }; \
static constexpr const std::array<std::string_view, kMaxEnumSize> EnumName##_enum_names = {\
    ALL_ARGUMENTS_TO_STRING(__VA_ARGS__)\
};\
inline constexpr size_t Get##EnumName##Size() noexcept {\
    return EnumName##_enum_names.size();\
}\
inline constexpr std::string_view EnumToString(EnumName value) noexcept {\
    size_t index = static_cast<size_t>(value); \
    return (index < EnumName##_enum_names.size() - 1) ? EnumName##_enum_names[index] : "Unknown";\
}\
inline std::ostream &operator<<(std::ostream &os, EnumName value) {\
    os << EnumToString(value);\
    return os;\
}

