//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <sstream>
#include <string>
#include <iomanip>
#include <algorithm>

#include <base/base.h>

namespace xamp::base {

XAMP_BASE_API std::wstring ToStdWString(std::string const& utf8);

XAMP_BASE_API std::string ToUtf8String(std::wstring const& utf16);

XAMP_ALWAYS_INLINE std::wstring ToString(std::string const& utf8) {
	return ToStdWString(utf8);
}

XAMP_ALWAYS_INLINE std::string ToString(std::wstring const& utf16) {
	return ToUtf8String(utf16);
}

template <typename CharType>
std::basic_string<CharType> ToUpper(std::basic_string<CharType> s) {
	std::transform(s.begin(), s.end(), s.begin(), ::toupper);
	return s;
}

template <typename CharType>
std::basic_string<CharType> ToLower(std::basic_string<CharType> s) {
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

XAMP_ALWAYS_INLINE std::string FormatBytes(size_t bytes) noexcept {
    static constexpr float tb = 1099511627776;
    static constexpr float gb = 1073741824;
    static constexpr float mb = 1048576;
    static constexpr float kb = 1024;

    char buffer[2048]{0};

    if (bytes >= tb)
        snprintf(buffer, sizeof(buffer), "%.2f TB", static_cast<float>(bytes) / tb);
    else if (bytes >= gb && bytes < tb)
        snprintf(buffer, sizeof(buffer), "%.2f GB", static_cast<float>(bytes) / gb);
    else if (bytes >= mb && bytes < gb)
        snprintf(buffer, sizeof(buffer), "%.2f MB", static_cast<float>(bytes) / mb);
    else if (bytes >= kb && bytes < mb)
        snprintf(buffer, sizeof(buffer), "%.2f KB", static_cast<float>(bytes) / kb);
    else if (bytes < kb)
        snprintf(buffer, sizeof(buffer), "%.2f B", static_cast<float>(bytes));
    else
        snprintf(buffer, sizeof(buffer), "%.2f B", static_cast<float>(bytes));

    return buffer;
}

template <typename T>
XAMP_ALWAYS_INLINE std::string FormatBytesBy(size_t bytes) noexcept {
    return FormatBytes(sizeof(T) * bytes);
}

}
