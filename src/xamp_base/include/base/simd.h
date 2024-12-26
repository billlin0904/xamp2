//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/assert.h>

#ifdef XAMP_OS_WIN
#include <intrin.h>
#include <xmmintrin.h>
#else
#include <cpuid.h>
#include <immintrin.h>
#endif

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN

#define m256  __m256
#define m256i __m256i
#define m128 __m128
#define m128i __m128i

#ifdef XAMP_OS_WIN
#define XAMP_VECTOR_CALL __vectorcall
#else
#define XAMP_VECTOR_CALL __attribute__((vectorcall))
#endif

inline constexpr int32_t kSSESimdLanes = sizeof(m128i);
inline constexpr int32_t kAVX2SimdLanes = sizeof(m256i);
// note: int/float = 4 Byte
inline constexpr int32_t kSimdAlignedSize = kAVX2SimdLanes / sizeof(float);
inline constexpr size_t kSimdCopyAlignedSize = 128;

class SIMD {
public:
    SIMD() = delete;

    XAMP_DISABLE_COPY(SIMD)

    static XAMP_ALWAYS_INLINE bool IsCPUSupportAVX2() {
        int32_t reg[4]{ 0 };
#ifdef XAMP_OS_WIN
        ::__cpuidex(reg, 7, 0);
#elif defined(XAMP_OS_MAC)
        __cpuid_count(7, 0, reg[0], reg[1], reg[2], reg[3]);
#endif
        return (reg[1] & (1 << 5)) != 0;
    }
};

#endif

XAMP_BASE_NAMESPACE_END
