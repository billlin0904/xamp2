#include <algorithm>
#include <vector>

#include <base/rng.h>

namespace xamp::base {

RNG& RNG::Instance() {
    static thread_local RNG rng;
    return rng;
}

RNG::RNG() {
    std::random_device device;
    std::vector<std::mt19937_64::result_type> seeds(std::mt19937_64::state_size);
    std::generate(std::begin(seeds), std::end(seeds), std::ref(device));
    std::seed_seq seq(std::begin(seeds), std::end(seeds));
    engine_.seed(seq);
}

}
