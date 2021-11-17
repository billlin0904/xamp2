//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <intrin.h>
#include <xmmintrin.h>

namespace xamp::base {

#ifdef XAMP_OPTIMIZE_AVX
#define __F32VEC_SIZE	32
#define __f32vec	__m256
#define __f32veci	__m256i
#elif defined(XAMP_OPTIMIZE_SSE2)
#define __F32VEC_SIZE	16
#define __f32vec	__m128
#define __f32veci	__m128i
#endif
   
template <int N>
XAMP_ALWAYS_INLINE __f32veci ShiftLeftBits(__f32veci src) {
#ifdef XAMP_OPTIMIZE_AVX
    return ::_mm256_slli_si256(src, N);
#elif defined(XAMP_OPTIMIZE_SSE2)
    return ::_mm_slli_si128(src, N);
#endif
}

template <int N = 0>
XAMP_ALWAYS_INLINE void F32ToS32Aligned(void* dst, __f32vec src) {
#ifdef XAMP_OPTIMIZE_AVX    
    _mm256_store_si256(reinterpret_cast<__f32veci*>(dst), ShiftLeftBits<N>(_mm256_cvtps_epi32(src)));
#elif defined(XAMP_OPTIMIZE_SSE2)
    _mm_store_si128(reinterpret_cast<__f32veci*>(dst), ShiftLeftBits<N>(_mm_cvtps_epi32(src)));
#endif
}

template <size_t N = 0>
XAMP_ALWAYS_INLINE void F32ToS32(void* dst, __f32vec src) {
#ifdef XAMP_OPTIMIZE_AVX
    _mm256_store_si256(dst, ShiftLeftBits<N>(_mm256_cvtps_epi32(src)));
#elif defined(XAMP_OPTIMIZE_SSE2)
    _mm_store_si128(dst, ShiftLeftBits<N>(_mm_cvtps_epi32(src)));
#endif
}

XAMP_ALWAYS_INLINE __f32vec F32VecLoadAligned(float const* ptr) {
#ifdef XAMP_OPTIMIZE_AVX
    return _mm256_load_ps(ptr);
#elif defined(XAMP_OPTIMIZE_SSE2)
    return _mm_load_si128(ptr);
#endif
}

XAMP_ALWAYS_INLINE __f32vec F32Set1Ps(float f32) {
#ifdef XAMP_OPTIMIZE_AVX
    return _mm256_set1_ps(f32);
#elif defined(XAMP_OPTIMIZE_SSE2)
    return _mm_set1_ps(f32);
#endif
}

XAMP_ALWAYS_INLINE __f32vec F32MulPs(__f32vec f1, __f32vec f2) {
#ifdef XAMP_OPTIMIZE_AVX
    return _mm256_mul_ps(f1, f2);
#elif defined(XAMP_OPTIMIZE_SSE2)
    return _mm_mul_ps1(f1, f2);
#endif
}

XAMP_ALWAYS_INLINE __f32vec F32MinPs(__f32vec f1, __f32vec f2) {
#ifdef XAMP_OPTIMIZE_AVX
    return _mm256_min_ps(f1, f2);
#elif defined(XAMP_OPTIMIZE_SSE2)
    return _mm_min_ps(f1, f2);
#endif
}

XAMP_ALWAYS_INLINE __f32vec F32MaxPs(__f32vec f1, __f32vec f2) {
#ifdef XAMP_OPTIMIZE_AVX
    return _mm256_max_ps(f1, f2);
#elif defined(XAMP_OPTIMIZE_SSE2)
    return _mm_max_ps(f1, f2);
#endif
}

}

#endif

