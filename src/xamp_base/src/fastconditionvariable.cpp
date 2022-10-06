#include <base/fastconditionvariable.h>

namespace xamp::base {

void FastConditionVariable::wait(std::unique_lock<FastMutex>& lock) {
	auto old_state = state_.load(std::memory_order_relaxed);
	lock.unlock();
    AtomicWait(state_, old_state, kInfinity);
	lock.lock();
}

void FastConditionVariable::notify_one() noexcept {
	state_.fetch_add(kLocked, std::memory_order_relaxed);
    AtomicWakeSingle(state_);
}

void FastConditionVariable::notify_all() noexcept {
	state_.fetch_add(kLocked, std::memory_order_relaxed);
    AtomicWakeAll(state_);
}

}
