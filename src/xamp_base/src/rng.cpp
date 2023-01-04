#include <base/rng.h>

#include <base/platform.h>

namespace xamp::base {

PRNG::PRNG() noexcept
    : engine_(GenRandomSeed()) {
}

void PRNG::SetSeed() {
	engine_.seed(GenRandomSeed());
}

std::string PRNG::GetRandomString(size_t size) {
    static constexpr char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::string temp;
    temp.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        temp += alphanum[NextInt32(0, sizeof(alphanum) - 1)];
    }
    return temp;
}

}
