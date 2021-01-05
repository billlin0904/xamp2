#include <base/waitabletimer.h>

namespace xamp::base {

WaitableTimer::WaitableTimer() noexcept
	: timeout_(0)
    , tp_(std::chrono::steady_clock::now()) {
}

void WaitableTimer::SetTimeout(std::chrono::milliseconds timeout) noexcept {
	timeout_ = timeout;
}

void WaitableTimer::Wait() noexcept {
	tp_ += timeout_;
	std::this_thread::sleep_until(tp_);
}

}
