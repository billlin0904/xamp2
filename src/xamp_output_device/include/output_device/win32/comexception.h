//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/exception.h>
#include <base/com_error_category.h>

#include <output_device/output_device.h>

#define HrIfNotEqualThrow(hresult, otherHr) \
	do { \
		if (FAILED((hresult)) && ((otherHr) != (hresult))) { \
			throw_translated_com_error(hresult); \
		} \
	} while (false)

#define HrIfFailThrow(hresult) \
	do { \
		if (FAILED((hresult))) { \
			throw_translated_com_error(hresult); \
		} \
	} while (false)

#endif
