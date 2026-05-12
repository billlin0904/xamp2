#include <base/stopwatch.h>

XAMP_BASE_NAMESPACE_BEGIN

Stopwatch::Stopwatch() {
	Reset();
}

void Stopwatch::Reset() {
	start_time_ = clock_.now();
}

XAMP_BASE_NAMESPACE_END
