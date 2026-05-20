#include <base/fastconditionvariable.h>

XAMP_BASE_NAMESPACE_BEGIN

void FastConditionVariable::wait(std::unique_lock<FastMutex>& lock) {
    const auto old_state = state_.load(std::memory_order_acquire);
    lock.unlock();
    AtomicWait(state_, old_state, kInfinity);
    lock.lock();
}

void FastConditionVariable::notify_one() {
    state_.fetch_add(1, std::memory_order_release);
    AtomicWakeSingle(state_);
}

void FastConditionVariable::notify_all() {
    state_.fetch_add(1, std::memory_order_release);
    AtomicWakeAll(state_);
}

XAMP_BASE_NAMESPACE_END
