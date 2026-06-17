#include <base/stopwatch.h>

XAMP_BASE_NAMESPACE_BEGIN

Stopwatch::Stopwatch() {
	reset();
}

void Stopwatch::reset() {
	start_time_ = clock_.now();
}

XAMP_BASE_NAMESPACE_END
