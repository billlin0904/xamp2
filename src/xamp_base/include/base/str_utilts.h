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
#include <vector>

#include <base/base.h>

namespace xamp::base::String {

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

template <typename CharType>
void LTrim(std::basic_string<CharType> &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](auto ch) {
                return !std::isspace(ch);
            }));
}

template <typename CharType>
void RTrim(std::basic_string<CharType> &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](auto ch) {
                return !std::isspace(ch);
            }).base(), s.end());
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
int _StringPrint(char *const buffer,
                size_t const buffer_count,
                char const *const format,
                Args const &... args) noexcept {
    auto const result = snprintf(buffer, buffer_count, format, Argument(args)...);
    return result;
}

template <typename... Args>
std::string StringPrint(const char *format, Args... args) {
    std::string buffer;
    auto size = _StringPrint(nullptr, 0, format, args...);
    buffer.resize(size);
    _StringPrint(&buffer[0], buffer.size() + 1, format, args...);
    return buffer;
}

template <typename  C>
std::string Join(C const& pieces, std::string_view const separator = ",") {
    std::string s;
    s.reserve(pieces.size() * 16);
    auto prev = std::prev(pieces.end());
	
    for (auto p = pieces.begin(); p != pieces.end(); ++p) {
        s += *p;
        if (p != prev) {
            s += separator;
        }            
    }
    return s;
}

XAMP_ALWAYS_INLINE std::vector<std::string_view> Split(std::string_view s, const std::string_view delims = " ") {
    std::vector<std::string_view> output;
    size_t first = 0;

    while (first < s.size()) {
        const auto second = s.find_first_of(delims, first);

        if (first != second) {
            output.emplace_back(s.substr(first, second - first));
        }            

        if (second == std::string_view::npos) {
            break;
        }
        first = second + 1;
    }
    return output;
}

}
