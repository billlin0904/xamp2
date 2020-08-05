//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <locale>
#include <codecvt>

#include <fstream>
#include <array>
#include <string>

#include <base/base.h>

namespace xamp::base {

XAMP_ALWAYS_INLINE bool TryImbue(std::wifstream& file, std::string_view name) noexcept {
	try {
		(void)file.imbue(std::locale(name.data()));
		return true;
	} catch (const std::exception&) {
		return false;
	}
}

XAMP_ALWAYS_INLINE void ImbueFileFromBom(std::wifstream& file) noexcept {
#ifdef XAMP_OS_WIN
	static constexpr std::array<std::string_view, 4> locale_names{
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
#else
	std::locale utf8_locale(std::locale(), new std::codecvt_utf8<wchar_t>());
	file.imbue(utf8_locale);
#endif
}

}
