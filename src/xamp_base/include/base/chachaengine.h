//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdlib>
#include <cstdint>
#include <array>

#include <base/base.h>

// note:
// https://gist.github.com/HarryR/53027823d9a0f9508fb418fd1053e234
// https://gist.github.com/imneme/f0fe8877e4deb3f6b9200a17c18bf155

namespace xamp::base {

class XAMP_BASE_API ChaChaEngine final {
public:
    static constexpr size_t kRounds = 20;
    static constexpr size_t kKeyWords = 8;
    static constexpr size_t kKeyBytes = (kKeyWords * sizeof(uint32_t));
    static constexpr size_t kStateWords = 16;
    static constexpr std::array<uint32_t, kKeyWords> kZeroKey = { 0 };

    typedef uint32_t result_type;

    ChaChaEngine() noexcept {
        Reseed(kZeroKey);
    }

    explicit ChaChaEngine(const std::array<uint32_t, kKeyWords> key) noexcept {
        Reseed(key);
    }

    void seed(uint64_t seed) {
    }

    void SetCounter(const uint64_t counter_low, const uint64_t counter_high) noexcept {
        state_[12] = (counter_low) & 0xFFFFFFFF;
        state_[13] = (counter_low >> 32) & 0xFFFFFFFF;
        state_[14] = (counter_high) & 0xFFFFFFFF;
        state_[15] = (counter_high >> 32) & 0xFFFFFFFF;
        index_ = kStateWords;
    }

    static constexpr result_type(min)() noexcept {
        return std::numeric_limits<result_type>::lowest();
    }

    static constexpr result_type(max)() noexcept {
        return (std::numeric_limits<result_type>::max)();
    }

    result_type operator()() noexcept {
        return NextU32();
    }

    uint32_t NextU32() noexcept {
        if (index_ == kStateWords) {
            Update();
        }
        return buffer_[index_++ % kStateWords];
    }

    uint64_t NextU64() noexcept {
        return (static_cast<uint64_t>(NextU32()) << 32) | NextU32();
    }

    friend bool operator ==(const ChaChaEngine& lhs, const ChaChaEngine& rhs) noexcept {
        for (int i = 0; i < kKeyWords; ++i) {
            if (lhs.state_[i] != rhs.state_[i]) return false;
        }

        return lhs.index_ == rhs.index_;
    }

    friend bool operator !=(const ChaChaEngine& lhs, const ChaChaEngine& rhs) noexcept {
        return !(lhs == rhs);
    }

private:
    void Reseed(const std::array<uint32_t, kKeyWords> key) noexcept {
        state_[0] = 0x61707865;
        state_[1] = 0x3320646E;
        state_[2] = 0x79622D32;
        state_[3] = 0x6B206574;

        for (size_t i = 0; i < kKeyWords; i++) {
            state_[4 + i] = key[i];
        }

        state_[12] = 0;
        state_[13] = 0;
        state_[14] = 0;
        state_[15] = 0;

        index_ = kStateWords;
    }

    void Update() noexcept {
        Core(buffer_, state_);

        index_ = 0;

        // Increment 128-bit counter, little-endian word order
        if (++state_[12] != 0) return;
        if (++state_[13] != 0) return;
        if (++state_[14] != 0) return;
        state_[15] += 1;
    }

    static void Core(std::array<uint32_t, kStateWords> &output, const std::array<uint32_t, kStateWords> input) noexcept {
#define CHACHA_ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define CHACHA_QUARTERROUND(x, a, b, c, d) \
            x[a] = x[a] + x[b]; x[d] ^= x[a]; x[d] = CHACHA_ROTL32(x[d], 16); \
            x[c] = x[c] + x[d]; x[b] ^= x[c]; x[b] = CHACHA_ROTL32(x[b], 12); \
            x[a] = x[a] + x[b]; x[d] ^= x[a]; x[d] = CHACHA_ROTL32(x[d],  8); \
            x[c] = x[c] + x[d]; x[b] ^= x[c]; x[b] = CHACHA_ROTL32(x[b],  7)

        output = input;

        for (size_t i = 0; i < kRounds; i += 2) {
            // Column round
            CHACHA_QUARTERROUND(output, 0, 4, 8, 12);
            CHACHA_QUARTERROUND(output, 1, 5, 9, 13);
            CHACHA_QUARTERROUND(output, 2, 6, 10, 14);
            CHACHA_QUARTERROUND(output, 3, 7, 11, 15);
            // Diagonal round
            CHACHA_QUARTERROUND(output, 0, 5, 10, 15);
            CHACHA_QUARTERROUND(output, 1, 6, 11, 12);
            CHACHA_QUARTERROUND(output, 2, 7, 8, 13);
            CHACHA_QUARTERROUND(output, 3, 4, 9, 14);
        }

#undef CHACHA_QUARTERROUND
#undef CHACHA_ROTL32

        for (size_t i = 0; i < kStateWords; i++) {
            output[i] += input[i];
        }
    }

    uint32_t index_{ kStateWords };
    std::array<uint32_t, kStateWords> buffer_{ 0 };
    std::array<uint32_t, kStateWords> state_{ 0 };
};

}

