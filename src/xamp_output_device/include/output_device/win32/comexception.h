//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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
	* Constructor.
	* 
	* @param[in] hresult: HRESULT
	* @param[in] expr: expression
	* @param[in] file_path: file path
	* @param[in] line_number: line number
	*/
	explicit ComException(long hresult, std::string_view expr = "");

	/*
	* Destructor.
	*/
	virtual ~ComException() override = default;

	/*
	* Get HRESULT.
	*/
	[[nodiscard]] long GetHResult() const;

	/*
	* Get expression.
	*/
	[[nodiscard]] const char* GetExpression() const noexcept override;

private:
	long hr_;
	std::string_view expr_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#define HrIfNotEqualThrow(hresult, otherHr) \
	do { \
		if (FAILED((hresult)) && ((otherHr) != (hresult))) { \
			throw ComException(hresult, #hresult); \
		} \
	} while (false)

#define HrIfFailThrow(hresult) \
	do { \
		if (FAILED((hresult))) { \
			throw ComException(hresult, #hresult); \
		} \
	} while (false)

#endif
