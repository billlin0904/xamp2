#include <base/fastmutex.h>

#ifdef XAMP_OS_WIN
#pragma comment(lib, "Synchronization.lib")
#elif defined(XAMP_OS_LINUX)
#include <sys/syscall.h>
#endif

namespace xamp::base {

XAMP_ALWAYS_INLINE bool OSFutexWait(std::atomic<uint32_t>& to_wait_on, uint32_t &expected, DWORD millseconds = INFINITE) noexcept {
#ifdef XAMP_OS_WIN
		return ::WaitOnAddress(&to_wait_on, &expected, sizeof(expected), millseconds);
#elif defined(XAMP_OS_LINUX)
		return ::syscall(SYS_futex, &to_wait_on, FUTEX_WAIT_PRIVATE, expected, nullptr, nullptr, 0) == -1;
#endif
}

template <typename T>
XAMP_ALWAYS_INLINE void OSFutexWakeSingle(std::atomic<T>& to_wake) noexcept {
#ifdef XAMP_OS_WIN
	::WakeByAddressSingle(&to_wake);
#elif defined(XAMP_OS_LINUX)
	::syscall(SYS_futex, &to_wake, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
#endif
}

template <typename T>
XAMP_ALWAYS_INLINE void OSFutexWakeAll(std::atomic<T>& to_wake) noexcept {
#ifdef XAMP_OS_WIN
	::WakeByAddressAll(&to_wake);
#elif defined(XAMP_OS_LINUX)
	::syscall(SYS_futex, &to_wake, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max(), nullptr, nullptr, 0);
#endif
}

int FastConditionVariable::_FutexWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, const struct timespec* to) noexcept {
#ifdef XAMP_OS_WIN
	if (to == nullptr) {
		OSFutexWait(to_wait_on, expected);
		return 0;
	}

	if (to->tv_nsec >= 1000000000) {
		errno = EINVAL;
		return -1;
	}

	if (to->tv_sec >= 2147) {
		OSFutexWait(to_wait_on, expected, 2147000000);
		return 0; /* time-out out of range, claim spurious wake-up */
	}

	const DWORD ms = (to->tv_sec * 1000000) + ((to->tv_nsec + 999) / 1000);

	if (!OSFutexWait(to_wait_on, expected, ms)) {
		errno = ETIMEDOUT;
		return -1;
	}
	return 0;
#elif defined(XAMP_OS_LINUX)
	return ::syscall(SYS_futex, &to_wait_on, FUTEX_WAIT_PRIVATE, expected, to, nullptr, 0);
#else
    return 0;
#endif
}

#ifdef XAMP_OS_WIN

void FastConditionVariable::wait(std::unique_lock<FastMutex>& lock) {
	auto old_state = state_.load(std::memory_order_relaxed);
	lock.unlock();
	OSFutexWait(state_, old_state);
	lock.lock();
}

void FastConditionVariable::notify_one() noexcept {
	state_.fetch_add(kLocked, std::memory_order_relaxed);
	OSFutexWakeSingle(state_);
}

void FastConditionVariable::notify_all() noexcept {
	state_.fetch_add(kLocked, std::memory_order_relaxed);
	OSFutexWakeSingle(state_);
}

SRWMutex::SRWMutex() noexcept {
	::InitializeSRWLock(&lock_);
}

void SRWMutex::lock() noexcept {
	::AcquireSRWLockExclusive(&lock_);
}

void SRWMutex::unlock() noexcept {
	::ReleaseSRWLockExclusive(&lock_);
}

bool SRWMutex::try_lock() noexcept {
	return ::TryAcquireSRWLockExclusive(&lock_);
}
#endif
}
