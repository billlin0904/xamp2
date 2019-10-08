//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/exception.h>

#include <output_device/win32/wasapi.h>
#include <output_device/device_type.h>

namespace xamp::output_device::win32 {

using namespace base;

class HRException final : public Exception {
public:
    HRException(HRESULT hresult, const char * expr = "");

	HRException(HRESULT hresult, Errors error);

	HRESULT GetHResult() const;

	const char* GetExpression() const override;

private:
	HRESULT hr_;
	const char* expr_;
};

}

#define HR_IF_FAILED_THROW2(hresult, otherHr) \
	do { \
		if (FAILED((hresult)) && ((otherHr) != (hresult))) { \
			throw HRException(hresult, #hresult); \
		} \
	} while (false)

#define HR_IF_FAILED_THROW(hresult) \
	do { \
		if (FAILED((hresult))) { \
			throw HRException(hresult, #hresult); \
		} \
	} while (false)