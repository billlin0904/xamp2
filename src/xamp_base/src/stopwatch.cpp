#include <base/stopwatch.h>

XAMP_BASE_NAMESPACE_BEGIN

Stopwatch::Stopwatch() noexcept {
	Reset();
}

void Stopwatch::Reset() noexcept {
	start_time_ = clock_.now();
}

XAMP_BASE_NAMESPACE_END
