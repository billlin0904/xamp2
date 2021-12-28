//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cmath>
#include <algorithm>

#include <base/base.h>

namespace xamp::base {

XAMP_BASE_API float VolumeToDb(int32_t volume) noexcept;

XAMP_BASE_API float LinearToLog(int32_t volume) noexcept;

XAMP_BASE_API int32_t LogToLineaer(float volume) noexcept;

}