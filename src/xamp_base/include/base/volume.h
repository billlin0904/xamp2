//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

namespace xamp::base {

XAMP_BASE_API float VolumeToDb(int32_t volume_level) noexcept;

XAMP_BASE_API float LinearToLog(int32_t volume_level) noexcept;

XAMP_BASE_API int32_t LogToLineaer(float volume_db) noexcept;

}