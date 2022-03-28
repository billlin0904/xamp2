//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
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

#define kSimdLanes 32
#define m256  __m256
#define m256i __m256i
#define m128 __m128
#define m128i __m128i

namespace xamp::base {

inline constexpr int32_t kFloat16Scale = 32767;
inline constexpr int32_t kFloat24Scale = 8388607;
// note: 必須要加上.f否則是round to 2147483648.
inline constexpr float kFloat32Scale = 2147483647.f;
inline constexpr float kMaxFloatSample = 1.0F;
inline constexpr float kMinFloatSample = -1.0F;

inline constexpr int32_t kFloatAlignedSize = kSimdLanes / sizeof(float);

class XAMP_BASE_API SIMD {
public:
    SIMD() = delete;

    XAMP_DISABLE_COPY(SIMD)

	static void MemoryCopyUnroll(void* d, const void* s, size_t n) {
        XAMP_ASSERT(n % 128 == 0);

        auto* dest_vec = static_cast<__m256i*>(d);
        const auto* src_vec = static_cast<const __m256i*>(s);
        size_t num_vec = n / sizeof(__m256i);

        for (; num_vec > 0; num_vec -= 4, src_vec += 4, dest_vec += 4) {
            _mm256_store_si256(dest_vec, _mm256_load_si256(src_vec));
            _mm256_store_si256(dest_vec + 1, _mm256_load_si256(src_vec + 1));
            _mm256_store_si256(dest_vec + 2, _mm256_load_si256(src_vec + 2));
            _mm256_store_si256(dest_vec + 3, _mm256_load_si256(src_vec + 3));
        }
    }

    static XAMP_ALWAYS_INLINE bool IsCPUSupportAVX2() {
        int32_t reg[4]{ 0 };
#ifdef XAMP_OS_WIN
        ::__cpuidex(reg, 7, 0);
#elif defined(XAMP_OS_MAC)
        __cpuid_count(7, 0, reg[0], reg[1], reg[2], reg[3]);
#endif
        return (reg[1] & (1 << 5)) != 0;
    }

    static XAMP_ALWAYS_INLINE bool IsAligned(const void* pointer) {
        return reinterpret_cast<uintptr_t>(pointer) % kSimdLanes == 0;
    }

    static XAMP_ALWAYS_INLINE m256i Truncate(const m256& val) {        
        return _mm256_cvttps_epi32(val);
    }

    static XAMP_ALWAYS_INLINE m256i Round(const m256& val) {        
        return _mm256_cvtps_epi32(val);
    }

    static XAMP_ALWAYS_INLINE void Store(void* dst, m256i val) {
        _mm256_store_si256(static_cast<m256i*>(dst), val);
    }

    template <int N>
    static XAMP_ALWAYS_INLINE m256i ShiftLeftBits(m256i src) {
        return _mm256_slli_si256(src, N);
    }

    static XAMP_ALWAYS_INLINE void F32ToS16(void* dst, m256 src) {
        auto temp = Round(src);
        // todo: 改用 _mm256_extractf128_si256
        temp = _mm256_packs_epi32(temp, _mm256_setzero_si256());
        temp = _mm256_permute4x64_epi64(temp, 0xD8);
        _mm_store_si128(static_cast<__m128i*>(dst), _mm256_castsi256_si128(temp));
    }

    template <size_t N = 0>
    static XAMP_ALWAYS_INLINE void F32ToS32(void* dst, m256 src) {
        Store(dst, ShiftLeftBits<N>(Round(src)));
    }

    static XAMP_ALWAYS_INLINE m256 LoadPs(float const* ptr) {
        return _mm256_load_ps(ptr);
    }

    static XAMP_ALWAYS_INLINE m256i LoadPs(int8_t const* ptr) {
        return _mm256_load_si256(reinterpret_cast<const m256i*>(ptr));
    }

    static XAMP_ALWAYS_INLINE m256 Set1Ps(float f32) {
        return _mm256_set1_ps(f32);
    }

    static XAMP_ALWAYS_INLINE m256 MulPs(const m256 & f1, const m256& f2) {
        return _mm256_mul_ps(f1, f2);
    }

    static XAMP_ALWAYS_INLINE m256 MinPs(m256 f1, m256 f2) {
        return _mm256_min_ps(f1, f2);
    }

    static XAMP_ALWAYS_INLINE m256 MaxPs(m256 f1, m256 f2) {
        return _mm256_max_ps(f1, f2);
    }

    static XAMP_ALWAYS_INLINE uint64_t GetSeed() {
        uint64_t seed = 0;
        _rdseed64_step(&seed);
        return seed;
    }
};

template <typename T>
struct Converter;

template <>
struct Converter<int8_t> {
    static XAMP_ALWAYS_INLINE m256i Shuffle(m256i v, m256i indexies) {
        return _mm256_shuffle_epi8(v, indexies);
    }

    static XAMP_ALWAYS_INLINE m256i Low(m256i lo, m256i hi) {
        return _mm256_unpacklo_epi8(lo, hi);
    }

    static XAMP_ALWAYS_INLINE m256i High(m256i lo, m256i hi) {
        return _mm256_unpackhi_epi8(lo, hi);
    }

    static XAMP_ALWAYS_INLINE __m256i CombineLow(m256i vect) {
        const __m256i zeros = _mm256_setzero_si256();
        return _mm256_unpacklo_epi8(vect, zeros);
    }

    static XAMP_ALWAYS_INLINE __m256i CombineHigh(m256i vect) {
        const __m256i zeros = _mm256_setzero_si256();
        return _mm256_unpackhi_epi8(vect, zeros);
    }

    static XAMP_ALWAYS_INLINE std::pair<m256, m256> ToInterleave(m256 a, m256 b) {
        auto lo = _mm256_unpacklo_epi8(_mm256_castps_si256(a), _mm256_castps_si256(b));
        auto hi = _mm256_unpacklo_epi8(_mm256_castps_si256(a), _mm256_castps_si256(b));
        return std::make_pair(
            _mm256_castsi256_ps(_mm256_permute2f128_si256(lo, hi, _MM_SHUFFLE(0,2,0,0))),
            _mm256_castsi256_ps(_mm256_permute2f128_si256(lo, hi, _MM_SHUFFLE(0,3,0,1)))
            );
    }

    static XAMP_ALWAYS_INLINE std::pair<m256i, m256i> ToPlanar(m256i a, m256i b) {
        static const m256i mask = _mm256_setr_epi8(
            1, 3, 5, 7, 9, 11, 13, 15, 0, 2, 4, 6, 8, 10, 12, 14,
            1, 3, 5, 7, 9, 11, 13, 15, 0, 2, 4, 6, 8, 10, 12, 14);
        const auto lo = Shuffle(a, mask);
        const auto hi = Shuffle(b, mask);
    	/*return std::make_pair(
            CombineLow(Low(lo, hi)),
            CombineHigh(High(lo, hi)));*/
        return std::make_pair(
            Low(lo, hi),
            High(lo, hi));
    }
};

template <>
struct Converter<float> {
    static XAMP_ALWAYS_INLINE m256 Shuffle(m256 v, m256i indexes) {
        return _mm256_permutevar8x32_ps(v, indexes);
    }

    static XAMP_ALWAYS_INLINE std::pair<m256, m256> ToInterleave(m256 a, m256 b) {
        auto lo = _mm256_unpacklo_epi32(_mm256_castps_si256(a), _mm256_castps_si256(b));
        auto hi = _mm256_unpacklo_epi32(_mm256_castps_si256(a), _mm256_castps_si256(b));
        return std::make_pair(
            _mm256_castsi256_ps(_mm256_permute2f128_si256(lo, hi, _MM_SHUFFLE(0,2,0,0))),
            _mm256_castsi256_ps(_mm256_permute2f128_si256(lo, hi, _MM_SHUFFLE(0,3,0,1)))
            );
        //return _mm256_permute2f128_ps(lo, hi, _MM_SHUFFLE(0,2,0,0));
    }

    static XAMP_ALWAYS_INLINE std::pair<m256, m256> ToPlanar(const m256& a, const m256& b) {
        static const m256i mask = _mm256_setr_epi32(0, 2, 4, 6, 1, 3, 5, 7);
        const auto lo = Shuffle(a, mask);
        const auto hi = Shuffle(b, mask);
        return std::make_pair(
            _mm256_permute2f128_ps(lo, hi, 0 | (2 << 4)),
            _mm256_permute2f128_ps(lo, hi, 1 | (3 << 4))
            );
    }
};

template <typename InputT, typename OutputT>
struct InterleaveToPlanar;

template <>
struct InterleaveToPlanar<int8_t, int8_t> {
    static XAMP_ALWAYS_INLINE void Convert(
        const int8_t* XAMP_RESTRICT in_hi,
        const int8_t* XAMP_RESTRICT in_low,
        int8_t* XAMP_RESTRICT out_hi,
        int8_t* XAMP_RESTRICT out_low) {
        const auto [fst, snd] = Converter<int8_t>::ToPlanar(
            SIMD::LoadPs(in_hi),
            SIMD::LoadPs(in_low));
        _mm256_storeu_si256(reinterpret_cast<m256i*>(out_hi), fst);
        _mm256_storeu_si256(reinterpret_cast<m256i*>(out_low), snd);
    }
};

template <>
struct InterleaveToPlanar<float, int32_t> {
    static XAMP_ALWAYS_INLINE void Convert(
        const float * XAMP_RESTRICT in_hi,
        const float * XAMP_RESTRICT in_low,
        int32_t * XAMP_RESTRICT out_hi,
        int32_t * XAMP_RESTRICT out_low,
        float mul = 1.0) {
        const auto scale = SIMD::Set1Ps(kFloat32Scale);
        const auto mul_scale = SIMD::Set1Ps(mul);
        const auto result = Converter<float>::ToPlanar(
            SIMD::MulPs(SIMD::LoadPs(in_hi), mul_scale),
            SIMD::MulPs(SIMD::LoadPs(in_low), mul_scale)
        );
        const auto temp1 = SIMD::MulPs(result.first, scale);
        const auto temp2 = SIMD::MulPs(result.second, scale);
        SIMD::Store(out_hi, SIMD::Truncate(temp1));
        SIMD::Store(out_low, SIMD::Truncate(temp2));
    }

    static XAMP_ALWAYS_INLINE void Convert(
        const float * XAMP_RESTRICT in,
        int32_t * XAMP_RESTRICT a,
        int32_t * XAMP_RESTRICT b,
        size_t len,
        float mul = 1.0) {
        constexpr auto offset = kFloatAlignedSize * 2;
        XAMP_ASSERT(len % offset == 0);

        for (size_t i = 0; i < len;) {
            auto hi = in;
            auto lo = in + kFloatAlignedSize;
            Convert(in, lo, a, b, mul);
            in += offset;
            a += kFloatAlignedSize;
            b += kFloatAlignedSize;
            i += offset;
        }
    }
};

}
