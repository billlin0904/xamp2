//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN
#define port_swscanf swscanf_s
#define port_sscanf  sscanf_s
#else
#define port_swscanf swscanf
#define port_sscanf  sscanf
#endif

XAMP_BASE_NAMESPACE_END

