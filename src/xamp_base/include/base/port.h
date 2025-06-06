//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <sstream>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN
#define port_swscanf swscanf_s
#define port_sscanf  sscanf_s
#define port_strcpy  strcpy_s
#else
#define port_swscanf swscanf
#define port_sscanf  sscanf
#define port_strcpy  strcpy
#endif

XAMP_BASE_NAMESPACE_END

