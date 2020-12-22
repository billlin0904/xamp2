//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cmath>
#include <base/base.h>

#ifdef XAMP_OS_WIN

namespace xamp::output_device {

XAMP_ALWAYS_INLINE float LinearToLog(int32_t volume) noexcept {
	// Windows volume db max(0) db, min (-96) db
	constexpr double MIN_DB = -96;

	if (volume == 0) {
		return 0.0;
	}
	
	auto db = (MIN_DB / 2) * ((100.0 - volume) / 100.0);
	return static_cast<float>(std::pow(10, db / 20));
}

}
#endif