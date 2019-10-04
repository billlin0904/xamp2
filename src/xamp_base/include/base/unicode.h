//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <base/base.h>

namespace xamp::base {

XAMP_BASE_API std::wstring ToStdWString(const std::string &utf8);

XAMP_BASE_API std::string ToUtf8String(const std::wstring &utf16);

XAMP_ALWAYS_INLINE std::wstring ToString(const std::string& utf8) {
	return base::ToStdWString(utf8);
}

XAMP_ALWAYS_INLINE std::string ToString(const std::wstring& utf16) {
	return base::ToUtf8String(utf16);
}

}
