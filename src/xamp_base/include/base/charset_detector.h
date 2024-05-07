//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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
private:
	class CharsetDetectorImpl;
	AlignPtr<CharsetDetectorImpl> impl_;
};

XAMP_BASE_NAMESPACE_END