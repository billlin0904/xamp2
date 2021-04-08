#pragma comment(lib, "Synchronization.lib")

#include <base/fastmutex.h>

namespace xamp::base {
#ifdef XAMP_OS_WIN

XAMP_ALWAYS_INLINE void FutexWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected) {
#ifdef _WIN32
	::WaitOnAddress(&to_wait_on, &expected, sizeof(expected), INFINITE);
#else
	::syscall(SYS_futex, &to_wait_on, FUTEX_WAIT_PRIVATE, expected, nullptr, nullptr, 0);
#endif
}

template <typename T>
XAMP_ALWAYS_INLINE void FutexWakeSingle(std::atomic<T>& to_wake) {
#ifdef _WIN32
	::WakeByAddressSingle(&to_wake);
#else
	::syscall(SYS_futex, &to_wake, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
#endif
}

template <typename T>
XAMP_ALWAYS_INLINE void FutexWakeAll(std::atomic<T>& to_wake) {
#ifdef _WIN32
	::WakeByAddressAll(&to_wake);
#else
	::syscall(SYS_futex, &to_wake, FUTEX_WAKE_PRIVATE, std::numeric_limits<int>::max(), nullptr, nullptr, 0);
#endif
}

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
		FutexWait(state_, kSleeper);
	}
}

void FutexMutex::unlock() noexcept {
	if (state_.exchange(kUnlocked, std::memory_order_release) == kSleeper) {
		FutexWakeSingle(state_);
	}
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

CriticalSection::CriticalSection() {
	if (!::InitializeCriticalSectionEx(&cs_, 1, CRITICAL_SECTION_NO_DEBUG_INFO)) {
		throw PlatformSpecException();
	}
	::SetCriticalSectionSpinCount(&cs_, 0);
}

CriticalSection::~CriticalSection() noexcept {
	::DeleteCriticalSection(&cs_);
}

void CriticalSection::lock() noexcept {
	::EnterCriticalSection(&cs_);
}

void CriticalSection::unlock() noexcept {
	::LeaveCriticalSection(&cs_);
}
	
using FastMutex = SRWMutex;
#else
using FastMutex = std::mutex;
#endif
}