//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <fstream>
#include <locale>
#include <codecvt>
#include <array>
#include <string>

#include <base/base.h>

namespace xamp::base {

XAMP_ALWAYS_INLINE bool IsUtf8(const std::wstring &str) noexcept {
    return str.length() >= 3 &&
        str[0] == 0xEF && str[1] == 0xBB && str[2] == 0xBF;
}

XAMP_ALWAYS_INLINE bool IsUtf16Be(const std::wstring &str) noexcept {
    return str.length() >= 2 &&
        str[0] == 0xFE && str[1] == 0xFF;
}

XAMP_ALWAYS_INLINE bool IsUtf16Le(const std::wstring &str) noexcept {
    return str.length() >= 2 &&
        str[0] == 0xFF && str[1] == 0xFE;
}

XAMP_ALWAYS_INLINE bool IsUtf32Be(const std::wstring &str) noexcept {
    return str.length() >= 4 &&
        str[0] == 0x00 && str[1] == 0x00 && str[2] == 0xFE && str[3] == 0xFF;
}

XAMP_ALWAYS_INLINE bool IsUtf32Le(const std::wstring &str) noexcept {
    return str.length() >= 4 &&
        str[0] == 0xFE && str[1] == 0xFF && str[2] == 0x00 && str[3] == 0x00;
}

// C++17 deprecated
#if 0
XAMP_ALWAYS_INLINE std::locale GetLocaleFromBom(const std::wstring &bom) noexcept {
    if (IsUtf8(bom)) {
        return std::locale(std::locale::empty(),
            new std::codecvt_utf8<wchar_t, 0x10ffff, std::consume_header>);
    } else if (IsUtf16Be(bom)) {
        return std::locale(std::locale::empty(),
            new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>);
    } else if (IsUtf16Le(bom)) {
		const auto le_bom = static_cast<std::codecvt_mode>(
            std::little_endian | std::consume_header);
        return std::locale(
            std::locale::empty(),
            new std::codecvt_utf16<wchar_t, 0x10ffff, le_bom>);
    }
    return std::locale();
}
#endif

XAMP_ALWAYS_INLINE bool TryImbue(std::wifstream& file, std::string_view name) {
	try {
		(void)file.imbue(std::locale(name.data()));
		return true;
	} catch (const std::exception&) {
		return false;
	}
}

XAMP_ALWAYS_INLINE void ImbueFileFromBom(std::wifstream& file) noexcept {
    constexpr std::array<std::string_view, 4> locale_names{
		"en_US.UTF-8",
		"zh_TW.UTF-8",
		"zh_CN.UTF-8",
		"ja_JP.SJIS",
	};

    std::wstring bom;
    if (std::getline(file, bom, L'\r')) {
		for (auto locale_name : locale_names) {
			if (TryImbue(file, locale_name)) {
				file.seekg(0, std::ios::beg);
				break;
			}
		}        
    }
}

}
