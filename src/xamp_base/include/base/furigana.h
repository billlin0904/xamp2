//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>

#include <base/base.h>
#include <base/memory.h>

XAMP_BASE_NAMESPACE_BEGIN

// Structure to represent a pair of Kanji text and its corresponding Furigana
struct XAMP_BASE_API FuriganaEntity {
	explicit FuriganaEntity(const std::wstring &text, const std::wstring &furigana = L"")
		: text(text)
		, furigana(furigana) {
	}
	std::wstring text;      // The Kanji or word text
	std::wstring furigana;  // The corresponding Furigana (reading)
};

class XAMP_BASE_API Furigana {
public:
	Furigana();

	XAMP_PIMPL(Furigana)

	std::vector<FuriganaEntity> Convert(const std::wstring& text);
private:
	class FuriganaImpl;
	ScopedPtr<FuriganaImpl> impl_;
};

XAMP_BASE_NAMESPACE_END
