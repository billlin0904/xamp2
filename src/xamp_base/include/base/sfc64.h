//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/math.h>
#include <limits>

XAMP_BASE_NAMESPACE_BEGIN

//
// Extremely fast random number generator that also produces very high quality random.
// The algorithm is based on the paper "SFC64: a fast 64-bit random number generator" by Sebastiano Vigna.
//      
// see PractRand: http://pracrand.sourceforge.net/PractRand.txt
//
template <int32_t TRotation = 24, int32_t TRightShift = 11, int32_t TLeftShift = 3>
class XAMP_BASE_API_ONLY_EXPORT Sfc64Engine final {
public:
    using state_type = uint64_t;
    using result_type = uint64_t;

    explicit Sfc64Engine(uint64_t init_seed = 0xcafef00dbeef5eedULL) noexcept
		: a_(0)
		, b_(0)
		, c_(0)
		, inc_(0) {
	    seed(init_seed);
    }

    Sfc64Engine(uint64_t seed1, uint64_t seed2, uint64_t seed3, int32_t warmup_round = 12) noexcept {
        seed(seed1, seed2, seed3, warmup_round);
    }

    void seed(uint64_t init_seed) noexcept {
        seed(init_seed, init_seed, init_seed);
    }

    void seed(uint64_t seed1, uint64_t seed2, uint64_t seed3, int32_t warmup_round = 12) noexcept {
        c_ = seed3;
        b_ = seed2;
        a_ = seed1;
        inc_ = 1;

        for (auto i = 0; i < warmup_round; ++i)
            operator()();
    }

    result_type operator()() noexcept {
        auto const tmp = a_ + b_ + inc_++;
        a_ = b_ ^ (b_ >> TRightShift);
        b_ = c_ + (c_ << TLeftShift);
    	c_= Rotl64(c_, TRotation) + tmp;
        return tmp;
    }

    static constexpr result_type(min)() noexcept {
        return std::numeric_limits<result_type>::lowest();
    }

    static constexpr result_type(max)() noexcept {
        return (std::numeric_limits<result_type>::max)();
    }

private:
    XAMP_BASE_API friend bool operator ==(const Sfc64Engine& lhs, const Sfc64Engine& rhs) noexcept {
        return (lhs.a_ == rhs.a_) && (lhs.b_ == rhs.b_)
            && (lhs.c_ == rhs.c_) && (lhs.inc_ == rhs.inc_);
    }

    XAMP_BASE_API friend bool operator !=(const Sfc64Engine& lhs, const Sfc64Engine& rhs) noexcept {
        return (lhs.a_ != rhs.a_)
        || (lhs.b_ != rhs.b_)
        || (lhs.c_ != rhs.c_)
    	|| (lhs.inc_ != rhs.inc_);
    }

    uint64_t a_;
    uint64_t b_;
    uint64_t c_;
    uint64_t inc_;
};

XAMP_BASE_NAMESPACE_END
