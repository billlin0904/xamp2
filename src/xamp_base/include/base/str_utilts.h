//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ostream>
#include <string>
#include <vector>
#include <type_traits>

#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>
#include <base/base.h>
#include <base/enum.h>

XAMP_BASE_NAMESPACE_BEGIN

namespace detail {

template <typename t>
struct IsAtomic : std::false_type {
};

template <typename t>
struct IsAtomic<std::atomic<t>> : std::true_type {
};

template <typename t>
XAMP_ALWAYS_INLINE decltype(auto) formatArgument(t&& value) {
	using ValueType = std::remove_cvref_t<t>;
	if constexpr (IsAtomic<ValueType>::value) {
		return value.load();
	}
	else if constexpr (requires { std::forward<t>(value).toString(); }) {
		return std::forward<t>(value).toString();
	}
	else if constexpr (std::is_enum_v<ValueType> && requires(ValueType enum_value) { enumToString(enum_value); }) {
		return enumToString(value);
	}
	else if constexpr (std::is_lvalue_reference_v<t&&>
		&& !std::is_arithmetic_v<ValueType>
		&& requires(std::ostream& os) { os << value; }) {
		return fmt::streamed(value);
	}
	else {
		return std::forward<t>(value);
	}
}

} // namespace detail

namespace String {

XAMP_BASE_API std::wstring toStdWString(std::string const& utf8);

XAMP_BASE_API std::string toUtf8String(std::wstring const& utf16);

XAMP_ALWAYS_INLINE std::wstring toString(std::string const& utf8) {
	return toStdWString(utf8);
}

XAMP_ALWAYS_INLINE std::string toString(std::wstring const& utf16) {
	return toUtf8String(utf16);
}

XAMP_ALWAYS_INLINE std::string asStdString(const std::string_view& s) {
    return { s.data(), s.size() };
}

XAMP_BASE_API std::string localeStringToUTF8(const std::string& str) noexcept;

template <typename CharType>
std::basic_string<CharType> toUpper(std::basic_string<CharType> s) {
	std::transform(s.begin(), s.end(), s.begin(), ::toupper);
	return s;
}

template <typename CharType>
std::basic_string<CharType> toLower(std::basic_string<CharType> s) {
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

template <typename CharType>
void ltrim(std::basic_string<CharType> &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](auto ch) {
                return !std::isspace(ch);
            }));
}

template <typename CharType>
void rtrim(std::basic_string<CharType> &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](auto ch) {
                return !std::isspace(ch);
            }).base(), s.end());
}

template <typename CharType>
void remove(std::basic_string<CharType>& s, const std::basic_string<CharType>& p) {
    auto n = p.length();

    for (auto i = s.find(p);
        i != std::basic_string<CharType>::npos;
        i = s.find(p))
        s.erase(i, n);
}

template <typename CharType>
void remove(std::basic_string<CharType>& s, const CharType* target) {
    std::basic_string<CharType> p(target);
    remove(s, p);
}

XAMP_BASE_API std::string formatBytes(size_t bytes);

XAMP_ALWAYS_INLINE std::string toBeautyHex(const void* data,
    size_t size,
    size_t max_size = 256,
    std::string_view separator = " ") {
    if (data == nullptr || size == 0 || max_size == 0) {
        return {};
    }

    const auto dump_size = (std::min)(size, max_size);
    const auto* bytes = static_cast<const uint8_t*>(data);

    std::string output;
    output.reserve(dump_size * (2 + separator.size()) + 16);
    for (size_t i = 0; i < dump_size; ++i) {
        if (i != 0) {
            output.append(separator);
        }
        fmt::format_to(std::back_inserter(output), "{:02X}", bytes[i]);
    }

    if (dump_size < size) {
        fmt::format_to(std::back_inserter(output), "{}...({}/{})",
            separator,
            dump_size,
            size);
    }
    return output;
}

template <typename t>
XAMP_ALWAYS_INLINE std::string toBeautyHex(const t* data,
    size_t count,
    size_t max_bytes = 256,
    std::string_view separator = " ") {
    return toBeautyHex(static_cast<const void*>(data), sizeof(t) * count, max_bytes, separator);
}

template <typename t>
XAMP_ALWAYS_INLINE std::string formatBytesBy(size_t bytes) {
    return formatBytes(sizeof(t) * bytes);
}

template <typename  C>
std::string join(C const& pieces, std::string_view const separator = ",") {
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

template <typename t>
XAMP_ALWAYS_INLINE std::vector<std::basic_string_view<t>> split(std::basic_string_view<t> s,
    const std::basic_string_view<t> delims = " ") {
    std::vector<std::basic_string_view<t>> output;
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

template <typename t>
XAMP_ALWAYS_INLINE std::vector<std::basic_string_view<t>> split(const t* s,
    const t* delims = " ") {
    return split(std::basic_string_view<t>(s), std::basic_string_view<t>(delims));
}

template <typename... Args>
XAMP_ALWAYS_INLINE std::string format(std::string_view s, Args &&...args) {
    return fmt::format(fmt::runtime(s), detail::formatArgument(std::forward<Args>(args))...);
}

}

XAMP_BASE_NAMESPACE_END
