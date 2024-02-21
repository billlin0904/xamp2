//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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

/*
* AsioException is the asio exception.
* 
*/
class XAMP_OUTPUT_DEVICE_API AsioException final : public Exception {
public:
	/*
	* Constructor
	* 
	* @param error: asio error
	*/
	explicit AsioException(Errors error);

	/*
	* Constructor
	* 
	* @param error: asio error
	*/
	explicit AsioException(ASIOError error);

	/*
	* Get error message
	* 
	* @param error: asio error
	* @return error message
	*/
	static std::string_view ErrorMessage(ASIOError error) noexcept;
};

#define LogAsioIfFailed(expr) \
	do { \
		auto result = expr; \
		if (result != ASE_OK) { \
			XAMP_LOG_ERROR(AsioException::ErrorMessage(result)); \
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
