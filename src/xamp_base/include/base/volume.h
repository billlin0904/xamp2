//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

XAMP_BASE_API float volumeLevelToDb(int32_t volume_level);

XAMP_BASE_API float volumeLevelToGain(int32_t volume_level);

XAMP_BASE_API int32_t gainToVolumeLevel(float volume_db);

XAMP_BASE_NAMESPACE_END