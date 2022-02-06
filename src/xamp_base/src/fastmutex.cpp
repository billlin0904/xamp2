#include <base/fastmutex.h>

#ifdef XAMP_OS_WIN
#pragma comment(lib, "Synchronization.lib")
#elif defined(XAMP_OS_LINUX)
#include <sys/syscall.h>
#else
extern "C" int __ulock_wait(uint32_t operation, void *addr, uint64_t value,
                            uint32_t timeout); /* timeout is specified in microseconds */
extern "C" int __ulock_wake(uint32_t operation, void *addr, uint64_t wake_value);
#define UL_COMPARE_AND_WAIT	1
#define ULF_WAKE_ALL 0x00000100
#define INFINITE -1
#endif

namespace xamp::base {

#if defined(XAMP_OS_WIN) || defined(XAMP_OS_MAC)

#if defined(XAMP_OS_MAC)
template <typename T>
static int MacOSFutexWake(std::atomic<T>& to_wake, bool notify_one) {
    return ::__ulock_wake(UL_COMPARE_AND_WAIT | (notify_one ? 0 : ULF_WAKE_ALL), &to_wake, 0);
}
#endif

template <typename T>
XAMP_ALWAYS_INLINE bool OSFutexWait(std::atomic<T>& to_wait_on, uint32_t &expected, uint32_t millseconds) {
#ifdef XAMP_OS_WIN
    return ::WaitOnAddress(&to_wait_on, &expected, sizeof(expected), millseconds);
#elif defined(XAMP_OS_MAC)
    return ::__ulock_wait(UL_COMPARE_AND_WAIT, &to_wait_on, expected, millseconds) >= 0;
#endif
}

template <typename T>
XAMP_ALWAYS_INLINE void OSFutexWakeSingle(std::atomic<T>& to_wake) {
#ifdef XAMP_OS_WIN
	::WakeByAddressSingle(&to_wake);
#elif defined (XAMP_OS_MAC)
    MacOSFutexWake(to_wake, true);
#endif
}

template <typename T>
XAMP_ALWAYS_INLINE void OSFutexWakeAll(std::atomic<T>& to_wake) {
#ifdef XAMP_OS_WIN
	::WakeByAddressAll(&to_wake);
#elif defined (XAMP_OS_MAC)
    MacOSFutexWake(to_wake, false);
#endif
}

int FastConditionVariable::Wait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, const struct timespec* to) noexcept {
#if defined(XAMP_OS_WIN) || defined(XAMP_OS_MAC)
	if (to == nullptr) {
        OSFutexWait(to_wait_on, expected, INFINITE);
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

    const uint32_t ms = (to->tv_sec * 1000000) + ((to->tv_nsec + 999) / 1000);

	if (!OSFutexWait(to_wait_on, expected, ms)) {
		errno = ETIMEDOUT;
		return -1;
	}
	return 0;
#else
    return 0;
#endif
}
#endif

#if defined(XAMP_OS_WIN) || defined (XAMP_OS_MAC)
void FastConditionVariable::wait(std::unique_lock<FastMutex>& lock) {
	auto old_state = state_.load(std::memory_order_relaxed);
	lock.unlock();
	OSFutexWait(state_, old_state, INFINITE);
	lock.lock();
}

void FastConditionVariable::notify_one() noexcept {
	state_.fetch_add(kLocked, std::memory_order_relaxed);
	OSFutexWakeSingle(state_);
}

void FastConditionVariable::notify_all() noexcept {
	state_.fetch_add(kLocked, std::memory_order_relaxed);
    OSFutexWakeAll(state_);
}

#ifdef XAMP_OS_WIN
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

#endif
}
