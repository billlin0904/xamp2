#include <base/rng.h>
#include <base/algorithm.h>
#include <base/platform.h>

XAMP_BASE_NAMESPACE_BEGIN

Sfc64Engine<> MakeRandomEngine() {
    Sfc64Engine<> engine;

    // https://github.com/sevmeyer/prng/blob/master/include/prng/prng.hpp
    static auto entropy = GetSystemEntropy();

    // Ensure that each instance uses a different seed.
    // Constant from https://en.wikipedia.org/wiki/RC5
    entropy += UINT64_C(0x9e3779b97f4a7c15);
    auto c = entropy;

    // Add possible entropy from the current time.
    using Clock = std::chrono::high_resolution_clock;
    auto b = static_cast<uint64_t>(Clock::now().time_since_epoch().count());

    // Add possible entropy from the address of this object.
    // This is most effective when ASLR is active.
    auto a = static_cast<uint64_t>(std::hash<decltype(&engine)>{}(&engine));

    engine.seed(a, b, c, 18);
    return engine;
}

PRNG::PRNG() noexcept
    : engine_(MakeRandomEngine()) {
}

void PRNG::SetSeed(uint64_t seed) {
    engine_.seed(GetTime_t<std::chrono::milliseconds>() + seed);
}

std::string PRNG::GetRandomString(size_t size) {
    static constexpr std::string_view alphanum =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::string temp;
    temp.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        temp += alphanum[NextInt32(0, alphanum.length() - 1)];
    }
    return temp;
}

XAMP_BASE_NAMESPACE_END