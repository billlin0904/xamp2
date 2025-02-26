//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/memory.h>

XAMP_BASE_NAMESPACE_BEGIN

class XAMP_BASE_API CharsetDetector {
public:
	CharsetDetector();

	XAMP_PIMPL(CharsetDetector)

	std::string Detect(const char* data, size_t size);

	std::string Detect(const std::string &str) {
		return Detect(str.data(), str.size());
	}

	bool IsJapanese(const std::wstring& text);
private:
	class CharsetDetectorImpl;
	ScopedPtr<CharsetDetectorImpl> impl_;
};


XAMP_BASE_API void LoadUcharDectLib();

XAMP_BASE_NAMESPACE_END