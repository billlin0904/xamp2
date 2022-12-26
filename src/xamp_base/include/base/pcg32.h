//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#ifdef XAMP_USE_BENCHMAKR

#include <base/base.h>

#ifdef XAMP_USE_PCG32_AVX
#include <base/simd.h>
#endif

#include <limits>

namespace xamp::base {

// note:
// https://github.com/wjakob/pcg32

inline constexpr uint64_t kPCG32DefaultState = UINT64_C(0x853c49e6748fea9b);
inline constexpr uint64_t kPCG32DefaultStream = UINT64_C(0xda3e39cb94b95bdb);
inline constexpr uint64_t kPCG32Mult = UINT64_C(0x5851f42d4c957f2d);

class XAMP_BASE_API PCG32Engine final {
public:
    using state_type = uint64_t;
    using result_type = uint32_t;

    PCG32Engine()
	    : state_(kPCG32DefaultState)
		, inc_(kPCG32DefaultStream) {
    }

    void seed(uint64_t init_state, uint64_t init_seq = 1) {
        state_ = 0U;
        inc_ = (init_seq << 1u) | 1u;
        operator()();
        state_ += init_state;
        operator()();
    }

    result_type operator()() noexcept {
        auto old_state = state_;
        state_ = old_state * kPCG32Mult + inc_;
#if 0
        auto xorshifted = static_cast<uint32_t>(((old_state >> 18u) ^ old_state) >> 27u);
        auto rot = static_cast<uint32_t>(old_state >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((~rot + 1u) & 31));
#else
    	// using the PCG-XSH-RS scheme.
        return static_cast<result_type>((old_state ^ (old_state >> 22)) >> (22 + (old_state >> 61)));
#endif
    }

    static constexpr result_type(min)() noexcept {
        return std::numeric_limits<result_type>::lowest();
    }

    static constexpr result_type(max)() noexcept {
        return (std::numeric_limits<result_type>::max)();
    }

private:
    XAMP_BASE_API friend bool operator ==(const PCG32Engine& lhs, const PCG32Engine& rhs) noexcept {
        return (lhs.state_ == rhs.state_) && (lhs.inc_ == rhs.inc_);
    }

    XAMP_BASE_API friend bool operator !=(const PCG32Engine& lhs, const PCG32Engine& rhs) noexcept {
        return (lhs.state_ != rhs.state_) || (lhs.inc_ != rhs.inc_);
    }

    uint64_t state_;
    uint64_t inc_;
};

#ifdef XAMP_USE_PCG32_AVX
class XAMP_BASE_API PCG32Avx2Engine final {
public:
    using result_type = m256i;

    PCG32Avx2Engine() {
		constexpr uint64_t init_state[8] = {
			kPCG32DefaultState, kPCG32DefaultState,
			kPCG32DefaultState, kPCG32DefaultState,
			kPCG32DefaultState, kPCG32DefaultState,
			kPCG32DefaultState, kPCG32DefaultState
		};

        constexpr uint64_t init_seq[8] = {
			1, 2, 3, 4, 5, 6, 7, 8
		};

		seed(init_state, init_seq);
	}

	void seed(const uint64_t init_state[8], const uint64_t init_seq[8]) {
		const __m256i one = _mm256_set1_epi64x((long long)1);

		state_[0] = state_[1] = _mm256_setzero_si256();

		inc_[0] = _mm256_or_si256(
			_mm256_slli_epi64(_mm256_load_si256((m256i*) &init_seq[0]), 1),
			one);

		inc_[1] = _mm256_or_si256(
			_mm256_slli_epi64(_mm256_load_si256((m256i*) &init_seq[4]), 1),
			one);

		step();

		state_[0] = _mm256_add_epi64(state_[0], _mm256_load_si256((m256i*) &init_state[0]));
		state_[1] = _mm256_add_epi64(state_[1], _mm256_load_si256((m256i*) &init_state[4]));
	}

    result_type operator()() noexcept {
        return step();
    }
private:
    m256i XAMP_VECTOR_CALL step() {
        const m256i pcg32_mult_l = _mm256_set1_epi64x(static_cast<long long>(kPCG32Mult & 0xffffffffu));
        const m256i pcg32_mult_h = _mm256_set1_epi64x(static_cast<long long>(kPCG32Mult >> 32));
        const m256i mask_l = _mm256_set1_epi64x(static_cast<long long>(0x00000000ffffffffull));
        const m256i shift0 = _mm256_set_epi32(7, 7, 7, 7, 6, 4, 2, 0);
        const m256i shift1 = _mm256_set_epi32(6, 4, 2, 0, 7, 7, 7, 7);
        const m256i const32 = _mm256_set1_epi32(32);

        m256i s0 = state_[0], s1 = state_[1];

        /* Extract low and high words for partial products below */
        m256i s0_l = _mm256_and_si256(s0, mask_l);
        m256i s0_h = _mm256_srli_epi64(s0, 32);
        m256i s1_l = _mm256_and_si256(s1, mask_l);
        m256i s1_h = _mm256_srli_epi64(s1, 32);

        /* Improve high bits using xorshift step */
        m256i s0s = _mm256_srli_epi64(s0, 18);
        m256i s1s = _mm256_srli_epi64(s1, 18);

        m256i s0x = _mm256_xor_si256(s0s, s0);
        m256i s1x = _mm256_xor_si256(s1s, s1);

        m256i s0xs = _mm256_srli_epi64(s0x, 27);
        m256i s1xs = _mm256_srli_epi64(s1x, 27);

        m256i xors0 = _mm256_and_si256(mask_l, s0xs);
        m256i xors1 = _mm256_and_si256(mask_l, s1xs);

        /* Use high bits to choose a bit-level rotation */
        m256i rot0 = _mm256_srli_epi64(s0, 59);
        m256i rot1 = _mm256_srli_epi64(s1, 59);

        /* 64 bit multiplication using 32 bit partial products :( */
        m256i m0_hl = _mm256_mul_epu32(s0_h, pcg32_mult_l);
        m256i m1_hl = _mm256_mul_epu32(s1_h, pcg32_mult_l);
        m256i m0_lh = _mm256_mul_epu32(s0_l, pcg32_mult_h);
        m256i m1_lh = _mm256_mul_epu32(s1_l, pcg32_mult_h);

        /* Assemble lower 32 bits, will be merged into one 256 bit vector below */
        xors0 = _mm256_permutevar8x32_epi32(xors0, shift0);
        rot0 = _mm256_permutevar8x32_epi32(rot0, shift0);
        xors1 = _mm256_permutevar8x32_epi32(xors1, shift1);
        rot1 = _mm256_permutevar8x32_epi32(rot1, shift1);

        /* Continue with partial products */
        m256i m0_ll = _mm256_mul_epu32(s0_l, pcg32_mult_l);
        m256i m1_ll = _mm256_mul_epu32(s1_l, pcg32_mult_l);

        m256i m0h = _mm256_add_epi64(m0_hl, m0_lh);
        m256i m1h = _mm256_add_epi64(m1_hl, m1_lh);

        m256i m0hs = _mm256_slli_epi64(m0h, 32);
        m256i m1hs = _mm256_slli_epi64(m1h, 32);

        m256i s0n = _mm256_add_epi64(m0hs, m0_ll);
        m256i s1n = _mm256_add_epi64(m1hs, m1_ll);

        m256i xors = _mm256_or_si256(xors0, xors1);
        m256i rot = _mm256_or_si256(rot0, rot1);

        state_[0] = _mm256_add_epi64(s0n, inc_[0]);
        state_[1] = _mm256_add_epi64(s1n, inc_[1]);

        /* Finally, rotate and return the result */
        m256i result = _mm256_or_si256(
            _mm256_srlv_epi32(xors, rot),
            _mm256_sllv_epi32(xors, _mm256_sub_epi32(const32, rot))
        );

        return result;
    }

	m256i state_[2];
	m256i inc_[2];
};
#endif

}

#endif
