//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <expected>

#include <base/base.h>
#include <base/memory.h>

XAMP_BASE_NAMESPACE_BEGIN

enum class TextEncodeingError {
	TEXT_ENCODING_NOT_FOUND_FILE,
	TEXT_ENCODING_EMPTY_FILE,
	TEXT_ENCODING_UNKNOWN_ENCDOING,
	TEXT_ENCODING_INPUT_STRING_EMPTY,
	TEXT_ENCODING_API_ERROR,
	TEXT_ENCODING_TO_WIDE_ERROR,
	TEXT_ENCODING_INPUT_STRING_UTF8
};

class XAMP_BASE_API TextEncoding {
public:
	static constexpr size_t kBufferSize = 4096;

	TextEncoding();

	XAMP_PIMPL(TextEncoding)

	std::expected<std::string, TextEncodeingError> ToUtf8String(const std::string& input_encoding,
			const std::string& input,
			size_t buf_size = kBufferSize,
			bool ignore_error = false);

	std::expected<std::string, TextEncodeingError> ToUtf8String(const std::string& input,
		size_t buf_size = kBufferSize,
		bool ignore_error = false);

	bool IsUtf8(const std::string& input);
private:
	class TextEncodingImpl;
	ScopedPtr<TextEncodingImpl> impl_;
};

XAMP_BASE_API void LoadLibIconvLib();

XAMP_BASE_NAMESPACE_END

