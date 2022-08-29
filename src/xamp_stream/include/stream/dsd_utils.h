//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

namespace xamp::stream {

inline constexpr int32_t kPcmSampleRate441{ 44100 };

XAMP_STREAM_API uint32_t GetDOPSampleRate(uint32_t dsd_speed);

}
