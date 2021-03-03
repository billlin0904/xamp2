//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cmath>
#include <complex>
#include <base/base.h>

namespace xamp::base {

inline constexpr float kPI = 3.14159265F;
	
inline constexpr float kMinDb = -96.0F;
inline constexpr float kMiBin = static_cast<float>(1e-9);
	
using Complex = std::complex<float>;
	
inline float Lin2Db(float power) noexcept {
    return power < kMiBin ? kMinDb : 10.0f * std::log10(power);
}

inline float Mag2Db(float amplitude) noexcept {
	return 2.0f * Lin2Db(amplitude);
}

inline float GetMagnitude(Complex const & c) noexcept {
	return std::hypot(c.imag(), c.real());
}
	
}
