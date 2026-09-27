//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/exception.h>
#include <base/platfrom_handle.h>

#ifdef XAMP_OS_WIN

XAMP_BASE_NAMESPACE_BEGIN

XAMP_BASE_API std::string translatedHrError(HRESULT hr);

XAMP_BASE_NAMESPACE_END

#endif