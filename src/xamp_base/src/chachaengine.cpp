#ifdef XAMP_USE_BENCHMAKR

#include <base/math.h>
#include <base/chachaengine.h>

namespace xamp::base {

void ChaChaEngine::Core(std::array<uint32_t, kStateWords>& output, const std::array<uint32_t, kStateWords> input) noexcept {
#define CHACHA_QUARTERROUND(x, a, b, c, d) \
            x[a] = x[a] + x[b]; x[d] ^= x[a]; x[d] = Rotl32(x[d], 16); \
            x[c] = x[c] + x[d]; x[b] ^= x[c]; x[b] = Rotl32(x[b], 12); \
            x[a] = x[a] + x[b]; x[d] ^= x[a]; x[d] = Rotl32(x[d],  8); \
            x[c] = x[c] + x[d]; x[b] ^= x[c]; x[b] = Rotl32(x[b],  7)

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

    for (size_t i = 0; i < kStateWords; i++) {
        output[i] += input[i];
    }
}

ChaChaEngine::ChaChaEngine() noexcept {
    Reseed(kZeroKey);
}

ChaChaEngine::ChaChaEngine(const std::array<uint32_t, kKeyWords> key) noexcept {
    Reseed(key);
}

void ChaChaEngine::seed(uint64_t seed, uint64_t stream) {
    index_ = 0;
    std::array<uint32_t, kKeyWords> key{};
    key[1] = seed >> 32;
    key[2] = key[3] = 0xDEADBEEF;
    key[4] = stream & 0xFFFFFFFFU;
    key[5] = stream >> 32;
    key[6] = key[7] = 0xDEADBEEF;
}

void ChaChaEngine::Reseed(const std::array<uint32_t, kKeyWords> key) noexcept {
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

void ChaChaEngine::Update() noexcept {
    Core(buffer_, state_);

    index_ = 0;

    // Increment 128-bit counter, little-endian word order
    if (++state_[12] != 0) return;
    if (++state_[13] != 0) return;
    if (++state_[14] != 0) return;
    state_[15] += 1;
}

void ChaChaEngine::SetCounter(const uint64_t counter_low, const uint64_t counter_high) noexcept {
    state_[12] = (counter_low) & 0xFFFFFFFF;
    state_[13] = (counter_low >> 32) & 0xFFFFFFFF;
    state_[14] = (counter_high) & 0xFFFFFFFF;
    state_[15] = (counter_high >> 32) & 0xFFFFFFFF;
    index_ = kStateWords;
}

}

#endif