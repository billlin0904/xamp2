#include <base/waitabletimer.h>

namespace xamp::base {

WaitableTimer::WaitableTimer() noexcept
	: timeout_(0)
    , tp_(std::chrono::steady_clock::now()) {
}

}
