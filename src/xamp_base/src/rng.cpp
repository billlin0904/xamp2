#include <base/rng.h>

namespace xamp::base {

PRNG& PRNG::GetInstance() noexcept {
    static thread_local PRNG rng;
    return rng;
}

PRNG::PRNG() noexcept
    : engine_(std::random_device{}()) {
}

}
