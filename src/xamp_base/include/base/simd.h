//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
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
