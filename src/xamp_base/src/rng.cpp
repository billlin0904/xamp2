#include <base/rng.h>

#include <base/platform.h>

namespace xamp::base {

PRNG::PRNG() noexcept
    : engine_(GenRandomSeed()) {
}

void PRNG::SetSeed() {
	engine_.seed(GenRandomSeed());
}

}
