//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/exception.h>
#include <stream/stream.h>

namespace xamp::stream {

using namespace base;

class XAMP_STREAM_API AvException final : public Exception {
public:
    explicit AvException(int32_t error);
};

#define AvIfFailedThrow(expr) \
	do { \
		auto error = (expr); \
		if (error != 0) { \
			throw AvException(error); \
		} \
	} while (false)

}

