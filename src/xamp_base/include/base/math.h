//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cmath>
#include <complex>
#include <algorithm>

#include <base/base.h>
#include <base/memory.h>

XAMP_BASE_NAMESPACE_BEGIN

/*
* Math constant.
*/
inline constexpr double XAMP_PI{ 3.14159265358979323846 };

/*
* Rotates bits to the left.
* 
* @param[in] x
* @param[in] shift
* @return uint64_t
*/
template <uint32_t TShiftBits>
uint64_t Rotl64(const uint64_t x) noexcept {
	const uint64_t left = x << TShiftBits;
	const uint64_t right = x >> (64 - TShiftBits);
	return left | right;
}

/*
* Rotates bits to the left.
* 
* @param[in] x
* @param[in] shift
* @return uint64_t
*/
XAMP_ALWAYS_INLINE uint64_t Rotl64(const uint64_t x, uint32_t shift) noexcept {
#ifdef XAMP_OS_WIN
	return _rotl64(x, shift);
#else
    return (x << shift) | (x >> (64 - shift));
#endif
}

/*
* Next power of two.
* 
* @param[in] v
* @return int32_t
*/
XAMP_ALWAYS_INLINE size_t NextPowerOfTwo(size_t v) noexcept {
	if (v == 0) return 1;
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

/*
* Is next power of two.
* 
* @param[in] v
* @return int32_t
*/
XAMP_ALWAYS_INLINE size_t IsPowerOfTwo(size_t v) noexcept {
	return v > 0 && !(v & (v - 1));
}

/*
* Round.
* 
* @param[in] a
* @return T
*/
template <typename T>
T Round(T a) {
    static_assert(std::is_floating_point_v<T>, "Round<T>: T must be floating point");
    return (a > 0) ? ::floor(a + static_cast<T>(0.5)) : ::ceil(a - static_cast<T>(0.5));
}

/*
* Round.
* 
* @param[in] a
* @param[in] places
* @return T
* @note places must be positive.
*/
template <typename T>
T Round(T a, int32_t places) {
    static_assert(std::is_floating_point_v<T>, "Round<T>: T must be floating point");
    const T shift = pow(static_cast<T>(10.0), places);
    return Round(a * shift) / shift;
}

//log10f is exactly log2(x)/log2(10.0f)
#define log10f_fast(x)  (log2f_approx(x) * 0.3010299956639812f)

// This is a fast approximation to log2()
// Y = C[0]*F*F*F + C[1]*F*F + C[2]*F + C[3] + E;
XAMP_ALWAYS_INLINE float log2f_approx(float X) noexcept {
	float Y, F;
	int E;
	F = frexpf(fabsf(X), &E);
	Y = 1.23149591368684f;
	Y *= F;
	Y += -4.11852516267426f;
	Y *= F;
	Y += 6.02197014179219f;
	Y *= F;
	Y += -3.13396450166353f;
	Y += E;
	return Y;
}

XAMP_BASE_NAMESPACE_END

