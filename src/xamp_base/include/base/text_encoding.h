//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/memory.h>

XAMP_BASE_NAMESPACE_BEGIN

class XAMP_BASE_API TextEncoding {
public:
	TextEncoding();

	XAMP_PIMPL(TextEncoding)

	std::string ToUtf8String(const std::string& encoding_name,
			const std::string& input,
			size_t buf_size = 4096,
			bool ignore_error = false);

	std::string ToUtf8String(const std::string& input,
		size_t buf_size,
		bool ignore_error);
private:
	class TextEncodingImpl;
	AlignPtr<TextEncodingImpl> impl_;
};

XAMP_BASE_API void LoadLibIconvLib();

XAMP_BASE_NAMESPACE_END

