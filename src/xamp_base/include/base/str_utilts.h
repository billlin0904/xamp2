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

XAMP_BASE_API std::wstring ToStdWString(const std::string &utf8);

XAMP_BASE_API std::string ToUtf8String(const std::wstring &utf16);

XAMP_ALWAYS_INLINE std::wstring ToString(const std::string& utf8) {
	return ToStdWString(utf8);
}

XAMP_ALWAYS_INLINE std::string ToString(const std::wstring& utf16) {
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

}
