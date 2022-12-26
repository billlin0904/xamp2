//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#ifdef XAMP_USE_BENCHMAKR

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

    ChaChaEngine() noexcept;

    explicit ChaChaEngine(std::array<uint32_t, kKeyWords> key) noexcept;

    void seed(uint64_t seed, uint64_t stream = 0);

    void SetCounter(const uint64_t counter_low, const uint64_t counter_high) noexcept;

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

    static constexpr result_type(min)() noexcept {
        return std::numeric_limits<result_type>::lowest();
    }

    static constexpr result_type(max)() noexcept {
        return (std::numeric_limits<result_type>::max)();
    }
private:
    static void Core(std::array<uint32_t, kStateWords>& output, const std::array<uint32_t, kStateWords> input) noexcept;

    void Reseed(const std::array<uint32_t, kKeyWords> key) noexcept;

    void Update() noexcept;

    XAMP_BASE_API friend bool operator ==(const ChaChaEngine& lhs, const ChaChaEngine& rhs) noexcept {
        for (int i = 0; i < kKeyWords; ++i) {
            if (lhs.state_[i] != rhs.state_[i]) return false;
        }

        return lhs.index_ == rhs.index_;
    }

    XAMP_BASE_API friend bool operator !=(const ChaChaEngine& lhs, const ChaChaEngine& rhs) noexcept {
        return !(lhs == rhs);
    }

    uint32_t index_{ kStateWords };
    std::array<uint32_t, kStateWords> buffer_{ 0 };
    std::array<uint32_t, kStateWords> state_{ 0 };
};

}

#endif