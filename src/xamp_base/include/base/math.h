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
inline constexpr float kLog2 = 1.44269504F;
inline constexpr float kLog10 = 0.434294482F;
inline constexpr float kSqrt2 = 1.41421356F;
inline constexpr float kMinDb = -96.0F;

inline size_t Log2(size_t value) noexcept {
    size_t result = 0;
    while ((1 << result) < value) {
        ++result;
    }
    return result;
}

inline float Power2Db(float power) noexcept {
    return 10 * std::log10(power);
}

inline float Power2Dbm(float power) noexcept {
    return 10 * std::log10(power) + 30.0F;
}

inline float Power2Loudness(float power) noexcept {
    if (power == 0.0F) {
        return kMinDb;
    }
    return 10 * std::log10(power) - 0.691F;
}

inline float Loudness2Power(float loudness) noexcept {
    return std::pow(10, (loudness + 0.691F) / 10.0F);
}

inline float GetMag(std::complex<float>& c) noexcept {
    return std::hypot(c.imag(), c.real());
}

inline float Db2Power(float db) noexcept {
    return std::pow(10, db / 10.0F);
}

inline float GetPhase(std::complex<float>& c) noexcept {
    return std::atan2(c.imag(), c.real());
}

inline float GetPower(std::complex<float>& c) noexcept {
    return static_cast<float>(std::pow(c.imag(), 2) + std::pow(c.real(), 2));
}

}
