//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

inline constexpr int32_t kPcmSampleRate441{ 44100 };
inline constexpr int32_t kPcmSampleRate48{  48000 };

XAMP_BASE_API uint32_t GetDOPSampleRate(uint32_t dsd_speed);

XAMP_BASE_NAMESPACE_END
