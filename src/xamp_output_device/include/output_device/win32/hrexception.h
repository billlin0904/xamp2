//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/exception.h>

#ifdef _WIN32
#include <output_device/win32/wasapi.h>
#include <output_device/device_type.h>

namespace xamp::output_device::win32 {

using namespace base;

class HRException final : public Exception {
public:
	static void ThrowFromHResult(HRESULT hresult, std::string_view expr);
	
	HRException(HRESULT hresult, std::string_view expr = "");

	HRException(HRESULT hresult, Errors error);

	HRESULT GetHResult() const;

	const char* GetExpression() const override;

private:
	HRESULT hr_;
	std::string_view expr_;
};

}

#define HrIfFailledThrow2(hresult, otherHr) \
	do { \
		if (FAILED((hresult)) && ((otherHr) != (hresult))) { \
			HRException::ThrowFromHResult(hresult, #hresult); \
		} \
	} while (false)

#define HrIfFailledThrow(hresult) \
	do { \
		if (FAILED((hresult))) { \
			HRException::ThrowFromHResult(hresult, #hresult); \
		} \
	} while (false)
#endif
