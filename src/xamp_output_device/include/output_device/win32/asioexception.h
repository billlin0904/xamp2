//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#if ENABLE_ASIO
#include <asiosys.h>
#include <asio.h>
#include <asiodrivers.h>

#include <base/logger.h>
#include <base/exception.h>
#include <output_device/output_device.h>

namespace xamp::output_device::win32 {

class XAMP_OUTPUT_DEVICE_API ASIOException final : public Exception {
public:
	explicit ASIOException(Errors error);

	explicit ASIOException(ASIOError error);

	static std::string_view ErrorMessage(ASIOError error) noexcept;
};

#define LogAsioIfFailed(expr) \
	do { \
		auto result = expr; \
		if (result != ASE_OK) { \
			XAMP_LOG_ERROR(ASIOException::ErrorMessage(result)); \
		} \
	} while (false)

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