//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cassert>
#include <base/base.h>

#if defined(_DEBUG) || !defined(NDEBUG)
#define XAMP_ASSERT(expr) assert(expr)
#else
#define XAMP_ASSERT(expr) ((void)0)
#endif

