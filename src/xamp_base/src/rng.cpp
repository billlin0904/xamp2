#include <base/rng.h>

namespace xamp::base {

RNG& RNG::GetInstance() {
    static thread_local RNG rng;
    return rng;
}

RNG::RNG() = default;

}
