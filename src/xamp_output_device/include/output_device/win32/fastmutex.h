//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#ifdef XAMP_OS_WIN

#pragma comment(lib, "Synchronization.lib")

#include <mutex>
#include <base/windows_handle.h>
#include <output_device/win32/hrexception.h>

namespace xamp::output_device::win32 {

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

// https://probablydance.com/2019/12/30/measuring-mutexes-spinlocks-and-how-bad-the-linux-scheduler-really-is/
class XAMP_CACHE_ALIGNED(kMallocAlignSize) SpinLock {
public:
	static constexpr auto kSpinCount = 16;
	
	SpinLock() = default;
	
	XAMP_DISABLE_COPY(SpinLock)

	void lock() noexcept {
		for (auto spin_count = 0; !try_lock(); ++spin_count) {
			if (spin_count < kSpinCount) {
				YieldProcessor();
			} else {
				std::this_thread::yield();
				spin_count = 0;
			}
		}
	}

	void unlock() noexcept {
		lock_.store(false, std::memory_order_release);
	}
	
private:
	bool try_lock() noexcept {
		return !lock_.load(std::memory_order_relaxed) &&
			!lock_.exchange(true, std::memory_order_acquire);
	}
	std::atomic<bool> lock_ = { false };
};

class FutexMutex {
public:
	FutexMutex() = default;

	XAMP_DISABLE_COPY(FutexMutex)
	
	void lock() noexcept {
		if (state_.exchange(kLocked, std::memory_order_acquire) == kUnlocked) {
			return;
		}
		
		while (state_.exchange(kSleeper, std::memory_order_acquire) != kUnlocked) {
			FutexWait(state_, kSleeper);
		}
	}
	
	void unlock() noexcept {
		if (state_.exchange(kUnlocked, std::memory_order_release) == kSleeper) {
			FutexWakeSingle(state_);
		}			
	}

private:
	std::atomic<uint32_t> state_{ kUnlocked };

	static constexpr uint32_t kUnlocked = 0;
	static constexpr uint32_t kLocked = 1;
	static constexpr uint32_t kSleeper = 2;
};

class SRWMutex {
public:
	SRWMutex() = default;

	XAMP_DISABLE_COPY(SRWMutex)
	
	void lock() noexcept {
		::AcquireSRWLockExclusive(&lock_);
	}
	
	void unlock() noexcept {
		::ReleaseSRWLockExclusive(&lock_);
	}

	[[nodiscard]] bool try_lock() noexcept {
		return ::TryAcquireSRWLockExclusive(&lock_);
	}
private:
	SRWLOCK lock_ = SRWLOCK_INIT;
};

class CriticalSection {
public:
	CriticalSection() {
		if (!::InitializeCriticalSectionEx(&cs_, 1, CRITICAL_SECTION_NO_DEBUG_INFO)) {
			throw PlatformSpecException();
		}
		::SetCriticalSectionSpinCount(&cs_, 0);
	}

	XAMP_DISABLE_COPY(CriticalSection)

	~CriticalSection() noexcept {
		::DeleteCriticalSection(&cs_);
	}

	void lock() noexcept {
		::EnterCriticalSection(&cs_);
	}
	
	void unlock() noexcept {
		::LeaveCriticalSection(&cs_);
	}

private:
	CRITICAL_SECTION cs_;
};

//typedef std::mutex FastMutex;
//typedef CriticalSection FastMutex;
typedef SRWMutex FastMutex;
	
}
#endif
