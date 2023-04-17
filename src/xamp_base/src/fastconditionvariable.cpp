#include <base/fastconditionvariable.h>

XAMP_BASE_NAMESPACE_BEGIN

void FastConditionVariable::wait(std::unique_lock<FastMutex>& lock) {
	auto* mutex = lock.mutex();
	// Get old state.
	auto old_state = state_.load(std::memory_order_relaxed);
	// Unlock the mutex.
	mutex->unlock();
	// Wait for the condition variable.
    AtomicWait(state_, old_state, kInfinity);
	// Lock the mutex again.
	mutex->lock();
}

void FastConditionVariable::notify_one() noexcept {
	// Add the locked bit.
	state_.fetch_add(kLocked, std::memory_order_relaxed);
	// Notify one thread.
    AtomicWakeSingle(state_);
}

void FastConditionVariable::notify_all() noexcept {
	// Add the locked bit.
	state_.fetch_add(kLocked, std::memory_order_relaxed);
	// Notify all threads.
    AtomicWakeAll(state_);
}

XAMP_BASE_NAMESPACE_END
