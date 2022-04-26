#include <base/platform.h>
#include <base/rng.h>

namespace xamp::base {

PRNG::PRNG() noexcept
    : engine_(GenRandomSeed()) {
}

void PRNG::SetSeed() {
	engine_.seed(GenRandomSeed());
}

}
