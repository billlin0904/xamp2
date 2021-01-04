//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cmath>
#include <algorithm>

#include <base/base.h>

#ifdef XAMP_OS_WIN

namespace xamp::output_device {

inline constexpr float kDbPerStep = 0.5f;

XAMP_ALWAYS_INLINE float VolumeToDb(int32_t volume) noexcept {
	volume = std::clamp(volume, 0, 100);	
	return -kDbPerStep * static_cast<float>(100 - volume);
}

XAMP_ALWAYS_INLINE float LinearToLog(int32_t volume) noexcept {
	volume = std::clamp(volume, 0, 100);
	constexpr float kDbConvert = -kDbPerStep * 2.302585093f / 20.0f;
	return volume ? exp(static_cast<float>(100 - volume) * kDbConvert) : 0;
}

}
#endif