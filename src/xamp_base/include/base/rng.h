//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <random>
#include <algorithm>

#include <base/base.h>

namespace xamp::base {

namespace detail {
    template <typename I, typename R, uint32_t A, uint32_t B, uint32_t C>
    class XoroshiroBase {
    protected:
        static constexpr uint32_t ITYPE_BITS = 8 * sizeof(I);
        static constexpr uint32_t RTYPE_BITS = 8 * sizeof(R);

        I s0_;
        I s1_;

        static inline I rotl(const I x, int32_t k) noexcept {
            return (x << k) | (x >> (ITYPE_BITS - k));
        }
    public:
        using itype = I;
        using result_type = R;

        static constexpr result_type (min)() noexcept {
            return 0;
        }

        static constexpr result_type (max)() noexcept {
            return ~result_type(0);
        }

        XoroshiroBase(itype s0 = itype(0xc1f651c67c62c6e0), itype s1 = itype(0x30d89576f866ac9f)) noexcept
            : s0_(s0)
            , s1_((s0 || s1) ? s1 : 1) {
        }

        void advance() noexcept {
            s1_ ^= s0_;
            s0_ = rotl(s0_, A) ^ s1_ ^ (s1_ << B);
            s1_ = rotl(s1_, C);
        }

        bool operator==(const XoroshiroBase& rhs) noexcept {
            return (s0_ == rhs.s0_) && (s1_ == rhs.s1_);
        }

        bool operator!=(const XoroshiroBase& rhs) noexcept {
            return !operator==(rhs);
        }
    };

    template <typename I, typename R, uint32_t A, uint32_t B, uint32_t C, I Mult1, uint32_t Orot, I Mult2>
    class XoroshiroStarStar : public XoroshiroBase<I, R, A, B, C> {
        using base = XoroshiroBase<I, R, A, B, C>;
    	
    public:
        using base::base;

        R operator()() noexcept {
            const I result_ss = base::rotl(base::s0_ * Mult1, Orot) * Mult2;
            base::advance();
            return result_ss >> (base::ITYPE_BITS - base::RTYPE_BITS);
        }
    };
}


using XoroshiroStarStar64 = detail::XoroshiroStarStar<uint64_t, uint64_t, 24, 16, 37, 5, 7, 9>;
using XoroshiroStarStar32 = detail::XoroshiroStarStar<uint32_t, uint32_t, 26, 9, 13, 0x9E3779BB, 5, 5>;
	
class XAMP_BASE_API RNG final {
public:
    static RNG& GetInstance();

    XAMP_DISABLE_COPY(RNG)

    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type* = nullptr>
    T operator()(T min, T max) noexcept {
        return std::uniform_real_distribution(min, max)(engine_);
    }

    template <typename T, typename std::enable_if<std::is_integral<T>::value, float>::type* = nullptr>
    T operator()(T min, T max) noexcept {
        return std::uniform_int_distribution(min, max)(engine_);
    }
private:
    RNG();
    XoroshiroStarStar64 engine_;
};

}
