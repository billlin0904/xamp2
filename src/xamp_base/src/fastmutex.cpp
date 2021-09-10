#include <base/fastmutex.h>

#ifdef XAMP_OS_WIN
#pragma comment(lib, "Synchronization.lib")
#elif defined(XAMP_OS_LINUX)
#include <sys/syscall.h>
#endif

namespace xamp::base {

static XAMP_ALWAYS_INLINE void _FutexWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected) {
#ifdef XAMP_OS_WIN
		::WaitOnAddress(&to_wait_on, &expected, sizeof(expected), INFINITE);
#elif defined(XAMP_OS_LINUX)
		::syscall(SYS_futex, &to_wait_on, FUTEX_WAIT_PRIVATE, expected, nullptr, nullptr, 0);
#endif
}

template <typename T>
XAMP_ALWAYS_INLINE void _FutexWakeSingle(std::atomic<T>& to_wake) {
#ifdef XAMP_OS_WIN
	::WakeByAddressSingle(&to_wake);
#elif defined(XAMP_OS_LINUX)
	::syscall(SYS_futex, &to_wake, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
#endif
}

template <typename T>
XAMP_ALWAYS_INLINE void _FutexWakeAll(std::atomic<T>& to_wake) {
#ifdef XAMP_OS_WIN
	::WakeByAddressAll(&to_wake);
#elif defined(XAMP_OS_LINUX)
	::syscall(SYS_futex, &to_wake, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max(), nullptr, nullptr, 0);
#endif
}

int _FutexWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, const struct timespec* to) {
#ifdef XAMP_OS_WIN
	if (to == nullptr) {
		_FutexWait(to_wait_on, expected);
		return 0;
	}

	if (to->tv_nsec >= 1000000000) {
		errno = EINVAL;
		return -1;
	}

	if (to->tv_sec >= 2147) {
		::WaitOnAddress(&to_wait_on, &expected, sizeof(expected), 2147000000);
		return 0; /* time-out out of range, claim spurious wake-up */
	}

	const DWORD ms = (to->tv_sec * 1000000) + ((to->tv_nsec + 999) / 1000);

	if (!::WaitOnAddress(&to_wait_on, &expected, sizeof(expected), ms)) {
		errno = ETIMEDOUT;
		return -1;
	}
	return 0;
#elif defined(XAMP_OS_LINUX)
	::syscall(SYS_futex, &to_wait_on, FUTEX_WAIT_PRIVATE, expected, to, nullptr, 0);
#else
    return 0;
#endif
}

#ifdef XAMP_OS_WIN

void SpinLock::lock() noexcept {
	for (auto spin_count = 0; !try_lock(); ++spin_count) {
		if (spin_count < kSpinCount) {
			YieldProcessor();
		}
		else {
			std::this_thread::yield();
			spin_count = 0;
		}
	}
}

void SpinLock::unlock() noexcept {
	lock_.store(false, std::memory_order_release);
}

void FutexMutex::lock() noexcept {
	if (state_.exchange(kLocked, std::memory_order_acquire) == kUnlocked) {
		return;
	}

	while (state_.exchange(kSleeper, std::memory_order_acquire) != kUnlocked) {
		_FutexWait(state_, kSleeper);
	}
}

void FutexMutex::unlock() noexcept {
	if (state_.exchange(kUnlocked, std::memory_order_release) == kSleeper) {
		_FutexWakeSingle(state_);
	}
}

void FutexMutexConditionVariable::wait(std::unique_lock<FastMutex>& lock) {
	auto old_state = state_.load(std::memory_order_relaxed);
	lock.unlock();
	_FutexWait(state_, old_state);
	lock.lock();
}

void FutexMutexConditionVariable::notify_one() noexcept {
	state_.fetch_add(kLocked, std::memory_order_relaxed);
	_FutexWakeSingle(state_);
}

void FutexMutexConditionVariable::notify_all() noexcept {
	state_.fetch_add(kLocked, std::memory_order_relaxed);
	_FutexWakeSingle(state_);
}

void SRWMutex::lock() noexcept {
	::AcquireSRWLockExclusive(&lock_);
}

void SRWMutex::unlock() noexcept {
	::ReleaseSRWLockExclusive(&lock_);
}

[[nodiscard]] bool SRWMutex::try_lock() noexcept {
	return ::TryAcquireSRWLockExclusive(&lock_);
}
#endif
}
