//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cmath>
#include <complex>
#include <algorithm>

#include <base/base.h>
#include <base/memory.h>

namespace xamp::base {

template <size_t N>
constexpr std::array<uint64_t, N> Splitmix64(uint64_t state) noexcept {
	std::array<uint64_t, N> seeds = {};
	std::generate(seeds.begin(), seeds.end(), [state]() mutable noexcept {
		uint64_t z = (state += UINT64_C(0x9e3779b97f4a7c15));
		z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
		z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
		return z ^ (z >> 31);
		});
	return seeds;
}

template <typename T>
static XAMP_ALWAYS_INLINE T FromUnaligned(const void* address) {
	static_assert(std::is_trivially_copyable_v<T>);
	T res{};
	MemoryCopy(&res, address, sizeof(res));
	return res;
}

template <uint32_t TShiftBits>
uint64_t Rotl64(const uint64_t x) noexcept {
	const uint64_t left = x << TShiftBits;
	const uint64_t right = x >> (64 - TShiftBits);
	return left | right;
}

static XAMP_ALWAYS_INLINE uint64_t Rotl64(const uint64_t x, uint32_t shift) noexcept {
    return (x << shift) | (x >> (64 - shift));
}

static XAMP_ALWAYS_INLINE uint32_t Rotl32(const uint32_t x, uint32_t shift) noexcept {
	return (x << shift) | (x >> (32 - shift));
}

static XAMP_ALWAYS_INLINE int32_t NextPowerOfTwo(int32_t v) noexcept {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

static XAMP_ALWAYS_INLINE int32_t IsPowerOfTwo(int32_t v) noexcept {
	return v > 0 && !(v & (v - 1));
}

template <typename T>
T Round(T a) {
    static_assert(std::is_floating_point_v<T>, "Round<T>: T must be floating point");
    return (a > 0) ? ::floor(a + static_cast<T>(0.5)) : ::ceil(a - static_cast<T>(0.5));
}

template <typename T>
T Round(T a, int32_t places) {
    static_assert(std::is_floating_point_v<T>, "Round<T>: T must be floating point");
    const T shift = pow(static_cast<T>(10.0), places);
    return Round(a * shift) / shift;
}

}

