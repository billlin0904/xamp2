#include <base/stopwatch.h>

namespace xamp::base {

Stopwatch::Stopwatch() noexcept {
	Reset();
}

void Stopwatch::Reset() noexcept {
	start_time_ = clock_.now();
}

}
