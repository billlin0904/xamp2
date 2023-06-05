//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/output_device.h>

#include <base/exception.h>
#include <base/fs.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

/*
* HRException is the exception for HRESULT.
* 
*/
class XAMP_OUTPUT_DEVICE_API ComException final : public PlatformException {
public:
	/*
	* Throw from HRESULT.
	* 
	* @param[in] hresult: HRESULT
	* @param[in] expr: expression
	* @param[in] file_path: file path
	* @param[in] line_number: line number
	*/
	static inline void ThrowFromHResult(long hresult, std::string_view expr = "", const Path& file_path = "", int32_t line_number = 0) {
		throw ComException(hresult, expr, file_path, line_number);
	}

	/*
	* Constructor.
	* 
	* @param[in] hresult: HRESULT
	* @param[in] expr: expression
	* @param[in] file_path: file path
	* @param[in] line_number: line number
	*/
	explicit ComException(long hresult, std::string_view expr = "", const Path& file_path = "", int32_t line_number = 0);

	/*
	* Destructor.
	*/
	virtual ~ComException() = default;

	/*
	* Get HRESULT.
	*/
	long GetHResult() const;

	/*
	* Get expression.
	*/
	const char* GetExpression() const noexcept override;

	/*
	* Get file name and line.
	* 
	* @return file name and line
	*/
	std::string GetFileNameAndLine() const;

private:
	long hr_;
	std::string_view expr_;
	std::string file_name_and_line_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#define HrIfFailledThrow2(hresult, otherHr) \
	do { \
		if (FAILED((hresult)) && ((otherHr) != (hresult))) { \
			ComException::ThrowFromHResult(hresult, #hresult, __FILE__, __LINE__); \
		} \
	} while (false)

#define HrIfFailledThrow(hresult) \
	do { \
		if (FAILED((hresult))) { \
			ComException::ThrowFromHResult(hresult, #hresult, __FILE__, __LINE__); \
		} \
	} while (false)

#endif
