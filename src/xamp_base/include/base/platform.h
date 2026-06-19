//====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/platform_win.h>
#elif defined(XAMP_OS_MAC)
#include <base/platform_mac.h>
#elif defined(XAMP_OS_LINUX)
#include <base/platform_linux.h>
#endif
