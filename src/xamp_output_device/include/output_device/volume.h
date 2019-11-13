//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cmath>

#include <base/base.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

// e^2.302585093 = e^ln(10) = 10;
constexpr float DBPerStep{ 0.50f };
constexpr float DBConvert{ -DBPerStep * 2.302585093f / 20.0f };
constexpr float DBConvertInverse{ 1.0f / DBConvert };

XAMP_NEVER_INLINE float LinearToLog(int32_t volume) noexcept {
	return volume ? std::exp(float(100 - volume) * DBConvert) : 0;
}

XAMP_NEVER_INLINE int32_t LogToLinear(float volume) noexcept {
	return volume ? 100 - int32_t(DBConvertInverse * std::log(volume) + 0.5) : 0;
}

}
