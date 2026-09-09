//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#if XAMP_OS_WIN

#include <asiosys.h>
#include <asio.h>
#include <asiodrivers.h>

#include <base/logger.h>
#include <base/exception.h>
#include <output_device/output_device.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

class XAMP_OUTPUT_DEVICE_API AsioException final : public Exception {
public:
	explicit AsioException(Errors error);

	explicit AsioException(ASIOError error);

	static std::string_view errorMessage(ASIOError error) ;
};

#define LogAsioIfFailed(expr) \
	do { \
		auto result = expr; \
		if (result != ASE_OK) { \
			XAMP_LOG_ERROR(AsioException::errorMessage(result)); \
		} \
	} while (false)

#define AsioIfFailedThrow(expr) \
	do { \
		auto result = expr; \
		if (result != ASE_OK) { \
			throw AsioException(result); \
		} \
	} while (false)

#define AsioIfFailedThrow2(expr, excepted) \
	do { \
		auto result = expr; \
		if (result != (excepted)) { \
			throw AsioException(result); \
		} \
	} while (false)

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif
