//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <expected>

#include <base/base.h>
#include <base/memory.h>

XAMP_BASE_NAMESPACE_BEGIN

enum class EncodingDetectorError {
	ENCODING_ERROR_UNKNOWN,
	ENCODING_DATA_ERROR,
};

class XAMP_BASE_API OpenCCConvert {
public:
	OpenCCConvert();

	XAMP_PIMPL(OpenCCConvert)

	void Load(const std::string& file_name, const std::string& file_path);

	std::wstring Convert(const std::wstring& text) const;
private:
	class OpenCCConvertImpl;
	ScopedPtr<OpenCCConvertImpl> impl_;
};

class XAMP_BASE_API EncodingDetector {
public:
	EncodingDetector();

	XAMP_PIMPL(EncodingDetector)

	std::expected<std::string, EncodingDetectorError> Detect(const char* data, size_t size);

	std::expected<std::string, EncodingDetectorError> Detect(const std::string &str) {
		return Detect(str.data(), str.size());
	}

private:
	class EncodingDetectorImpl;
	ScopedPtr<EncodingDetectorImpl> impl_;
};

class XAMP_BASE_API LanguageDetector {
public:
	LanguageDetector();

	XAMP_PIMPL(LanguageDetector)

	bool IsJapanese(const std::wstring& text);

	bool IsChinese(const std::wstring& text);
private:
	class LanguageDetectorImpl;
	ScopedPtr<LanguageDetectorImpl> impl_;
};


XAMP_BASE_API void LoadUcharDectLib();

XAMP_BASE_NAMESPACE_END