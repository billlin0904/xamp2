//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/exception.h>
#include <output_device/win32/wasapi.h>

namespace xamp::output_device::win32 {

using namespace base;

class HRException final : public PlatformSpecException {
public:
	static void ThrowFromHResult(HRESULT hresult, std::string_view expr);

	static std::string_view ErrorToString(HRESULT hr) noexcept;

	static std::string ErrorToStringHelper(HRESULT hr);

	explicit HRException(HRESULT hresult, std::string_view expr = "");

	HRESULT GetHResult() const;

	const char* GetExpression() const noexcept override;

private:
	HRESULT hr_;
	std::string_view expr_;
};

}

#define LogHrFailled(expr) \
	do {\
		auto hr = expr;\
		if (FAILED(hr)) {\
			XAMP_LOG_DEBUG("{}", HRException::ErrorToStringHelper(hr));\
		}\
	} while(false)

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
