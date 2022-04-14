#include <base/platform.h>
#include <base/rng.h>

namespace xamp::base {

PRNG& PRNG::GetInstance() noexcept {
    static thread_local PRNG rng;
    return rng;
}

PRNG::PRNG() noexcept
    : engine_(GenRandom()) {
}

}
