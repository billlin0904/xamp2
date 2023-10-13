#include <base/rng.h>
#include <base/algorithm.h>
#include <base/platform.h>

namespace xamp::base {

PRNG::PRNG() noexcept
    : engine_(GenRandomSeed()) {
}

void PRNG::SetSeed() {
	engine_.seed(GenRandomSeed());
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

}
