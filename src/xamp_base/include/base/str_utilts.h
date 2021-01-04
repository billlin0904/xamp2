//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
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

XAMP_BASE_API std::string FormatBytes(size_t bytes) noexcept;

template <typename T>
XAMP_ALWAYS_INLINE std::string FormatBytesBy(size_t bytes) noexcept {
    return FormatBytes(sizeof(T) * bytes);
}

template <typename T>
T Argument(T value) noexcept {
    return value;
}

template <typename T>
T const *Argument(std::basic_string<T> const &value) noexcept {
    return value.c_str();
}

template <typename... Args>
int StringPrint(char *const buffer,
                size_t const bufferCount,
                char const *const format,
                Args const &... args) noexcept {
    auto const result = snprintf(buffer, bufferCount, format, Argument(args)...);
    return result;
}

template <typename... Args>
std::string StringPrint(const char *format, Args... args) {
    std::string buffer;
    auto size = StringPrint(nullptr, 0, format, args...);
    buffer.resize(size);
    StringPrint(&buffer[0], buffer.size() + 1, format, args...);
    return buffer;
}

}
