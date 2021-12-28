#include <base/rng.h>

namespace xamp::base {

PRNG& PRNG::GetInstance() {
    static thread_local PRNG rng;
    return rng;
}

PRNG::PRNG()
    : engine_(std::random_device{}()) {
}

}
