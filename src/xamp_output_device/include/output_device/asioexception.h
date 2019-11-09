//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#if ENABLE_ASIO
#include <asiosys.h>
#include <asio.h>
#include <asiodrivers.h>

#include <base/exception.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

using namespace base;

class XAMP_OUTPUT_DEVICE_API ASIOException final : public Exception {
public:
	explicit ASIOException(Errors error);

	explicit ASIOException(ASIOError error);
};

#define AsioIfFailedThrow(expr) \
	do { \
		auto result = expr; \
		if (result != ASE_OK) { \
			throw ASIOException(result); \
		} \
	} while (false)

#define AsioIfFailedThrow2(expr, excepted) \
	do { \
		auto result = expr; \
		if (result != (excepted)) { \
			throw ASIOException(result); \
		} \
	} while (false)

}
#endif
