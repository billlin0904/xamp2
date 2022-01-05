//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdlib>
#include <cstdint>
#include <array>

#include <base/base.h>

namespace xamp::base {

// note:
// https://www.pcg-random.org/posts/a-quick-look-at-xoshiro256.html

inline constexpr uint64_t kXoshiroDefaultSeed = UINT64_C(1234567890);

constexpr uint64_t Rotl(const uint64_t x, int32_t k) noexcept {
    return (x << k) | (x >> (64 - k));
}

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

inline constexpr std::array<uint64_t, 4> kJump{
    UINT64_C(0x180ec6d33cfd0aba),
    UINT64_C(0xd5a61266f0c9392c),
    UINT64_C(0xa9582618e03fc9aa),
    UINT64_C(0x39abdc4529b1661c)
};

inline constexpr std::array<uint64_t, 4> kLongJump{
    UINT64_C(0x76e15d3efefdcbbf),
    UINT64_C(0xc5004e441c522fb3),
    UINT64_C(0x77710069854ee241),
    UINT64_C(0x39109bb02acbe635)
};

class XAMP_BASE_API Xoshiro256StarStarEngine final {
public:
    using state_type = std::array<uint64_t, 4>;
    using result_type = uint64_t;

    explicit Xoshiro256StarStarEngine(uint64_t seed = kXoshiroDefaultSeed)
        : state_(Splitmix64<4>(seed)) {
    }

    void seed(uint64_t seed) {
        state_ = Splitmix64<4>(seed);
    }

    constexpr result_type operator()() noexcept {
        const auto result = Rotl(state_[1] * 5, 7) * 9;
        const auto t = state_[1] << 17;
        state_[2] ^= state_[0];
        state_[3] ^= state_[1];
        state_[1] ^= state_[2];
        state_[0] ^= state_[3];
        state_[2] ^= t;
        state_[3] = Rotl(state_[3], 45);
        return result;
    }

    constexpr void Jump() noexcept {
        uint64_t s0 = 0;
        uint64_t s1 = 0;
        uint64_t s2 = 0;
        uint64_t s3 = 0;

        for (const auto jump : kJump) {
            for (auto b = 0; b < 64; ++b) {
                if (jump & UINT64_C(1) << b) {
                    s0 ^= state_[0];
                    s1 ^= state_[1];
                    s2 ^= state_[2];
                    s3 ^= state_[3];
                }
                operator()();
            }
        }

        state_[0] = s0;
        state_[1] = s1;
        state_[2] = s2;
        state_[3] = s3;
    }

    constexpr void LongJump() noexcept {
        uint64_t s0 = 0;
        uint64_t s1 = 0;
        uint64_t s2 = 0;
        uint64_t s3 = 0;

        for (const auto jump : kLongJump) {
            for (auto b = 0; b < 64; ++b) {
                if (jump & UINT64_C(1) << b) {
                    s0 ^= state_[0];
                    s1 ^= state_[1];
                    s2 ^= state_[2];
                    s3 ^= state_[3];
                }
                operator()();
            }
        }

        state_[0] = s0;
        state_[1] = s1;
        state_[2] = s2;
        state_[3] = s3;
    }

    static constexpr result_type(min)() noexcept {
        return std::numeric_limits<result_type>::lowest();
    }

    static constexpr result_type(max)() noexcept {
        return (std::numeric_limits<result_type>::max)();
    }

    friend bool operator ==(const Xoshiro256StarStarEngine& lhs, const Xoshiro256StarStarEngine& rhs) noexcept {
        return (lhs.state_ == rhs.state_);
    }

    friend bool operator !=(const Xoshiro256StarStarEngine& lhs, const Xoshiro256StarStarEngine& rhs) noexcept {
        return (lhs.state_ != rhs.state_);
    }
private:
    state_type state_;
};

class XAMP_BASE_API Xoshiro256PlusEngine final {
public:
    using state_type = std::array<uint64_t, 4>;
    using result_type = uint64_t;

    explicit Xoshiro256PlusEngine(uint64_t seed = kXoshiroDefaultSeed)
        : state_(Splitmix64<4>(seed)) {
    }

    constexpr result_type operator()() noexcept {
        const auto result = state_[0] + state_[3];
        const auto t = state_[1] << 17;
        state_[2] ^= state_[0];
        state_[3] ^= state_[1];
        state_[1] ^= state_[2];
        state_[0] ^= state_[3];
        state_[2] ^= t;
        state_[3] = Rotl(state_[3], 45);
        return result;
    }

    void seed(uint64_t seed) {
        state_ = Splitmix64<4>(seed);
    }

    constexpr void Jump() noexcept {
        uint64_t s0 = 0;
        uint64_t s1 = 0;
        uint64_t s2 = 0;
        uint64_t s3 = 0;

        for (const auto jump : kJump) {
            for (auto b = 0; b < 64; ++b) {
                if (jump & UINT64_C(1) << b) {
                    s0 ^= state_[0];
                    s1 ^= state_[1];
                    s2 ^= state_[2];
                    s3 ^= state_[3];
                }
                operator()();
            }
        }

        state_[0] = s0;
        state_[1] = s1;
        state_[2] = s2;
        state_[3] = s3;
    }

    /*constexpr*/ void LongJump() noexcept {
        uint64_t s0 = 0;
        uint64_t s1 = 0;
        uint64_t s2 = 0;
        uint64_t s3 = 0;

        for (const auto jump : kLongJump) {
            for (auto b = 0; b < 64; ++b) {
                if (jump & UINT64_C(1) << b) {
                    s0 ^= state_[0];
                    s1 ^= state_[1];
                    s2 ^= state_[2];
                    s3 ^= state_[3];
                }
                operator()();
            }
        }

        state_[0] = s0;
        state_[1] = s1;
        state_[2] = s2;
        state_[3] = s3;
    }

    static constexpr result_type(min)() noexcept {
        return std::numeric_limits<result_type>::lowest();
    }

    static constexpr result_type(max)() noexcept {
        return (std::numeric_limits<result_type>::max)();
    }

    friend bool operator ==(const Xoshiro256PlusEngine& lhs, const Xoshiro256PlusEngine& rhs) noexcept {
        return (lhs.state_ == rhs.state_);
    }

    friend bool operator !=(const Xoshiro256PlusEngine& lhs, const Xoshiro256PlusEngine& rhs) noexcept {
        return (lhs.state_ != rhs.state_);
    }
private:
    state_type state_;
};

class XAMP_BASE_API Xoshiro256PlusPlusEngine final {
public:
    using state_type = std::array<uint64_t, 4>;
    using result_type = uint64_t;

    explicit Xoshiro256PlusPlusEngine(uint64_t seed = kXoshiroDefaultSeed)
        : state_(Splitmix64<4>(seed)) {
    }

    constexpr result_type operator()() noexcept {
        const auto result = Rotl(state_[0] + state_[3], 23) + state_[0];
        const auto t = state_[1] << 17;
        state_[2] ^= state_[0];
        state_[3] ^= state_[1];
        state_[1] ^= state_[2];
        state_[0] ^= state_[3];
        state_[2] ^= t;
        state_[3] = Rotl(state_[3], 45);
        return result;
    }

    void seed(uint64_t seed) {
        state_ = Splitmix64<4>(seed);
    }

    constexpr void Jump() noexcept {
        uint64_t s0 = 0;
        uint64_t s1 = 0;
        uint64_t s2 = 0;
        uint64_t s3 = 0;

        for (const auto jump : kJump) {
            for (auto b = 0; b < 64; ++b) {
                if (jump & UINT64_C(1) << b) {
                    s0 ^= state_[0];
                    s1 ^= state_[1];
                    s2 ^= state_[2];
                    s3 ^= state_[3];
                }
                operator()();
            }
        }

       state_[0] = s0;
       state_[1] = s1;
       state_[2] = s2;
       state_[3] = s3;
    }

    constexpr void LongJump() noexcept {
        uint64_t s0 = 0;
        uint64_t s1 = 0;
        uint64_t s2 = 0;
        uint64_t s3 = 0;

        for (const auto jump : kLongJump) {
            for (auto b = 0; b < 64; ++b) {
                if (jump & UINT64_C(1) << b) {
                    s0 ^= state_[0];
                    s1 ^= state_[1];
                    s2 ^= state_[2];
                    s3 ^= state_[3];
                }
                operator()();
            }
        }

        state_[0] = s0;
        state_[1] = s1;
        state_[2] = s2;
        state_[3] = s3;
    }

    static constexpr result_type (min)() noexcept {
        return std::numeric_limits<result_type>::lowest();
    }

    static constexpr result_type (max)() noexcept {
        return (std::numeric_limits<result_type>::max)();
    }

    friend bool operator ==(const Xoshiro256PlusPlusEngine& lhs, const Xoshiro256PlusPlusEngine& rhs) noexcept {
        return (lhs.state_ == rhs.state_);
    }

    friend bool operator !=(const Xoshiro256PlusPlusEngine& lhs, const Xoshiro256PlusPlusEngine& rhs) noexcept {
        return (lhs.state_ != rhs.state_);
    }
private:
    state_type state_;
};

}

