//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <mutex>
#include <base/windows_handle.h>
#include <base/exception.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN

// https://probablydance.com/2019/12/30/measuring-mutexes-spinlocks-and-how-bad-the-linux-scheduler-really-is/
class XAMP_BASE_API XAMP_CACHE_ALIGNED(kMallocAlignSize) SpinLock {
public:
	static constexpr auto kSpinCount = 16;
	
	SpinLock() = default;
	
	XAMP_DISABLE_COPY(SpinLock)

	void lock() noexcept;

	void unlock() noexcept;
	
private:
	bool try_lock() noexcept {
		return !lock_.load(std::memory_order_relaxed) &&
			!lock_.exchange(true, std::memory_order_acquire);
	}
	std::atomic<bool> lock_ = { false };
};

class XAMP_BASE_API FutexMutex {
public:
	FutexMutex() = default;

	XAMP_DISABLE_COPY(FutexMutex)

	void lock() noexcept;
	
	void unlock() noexcept;

private:
	std::atomic<uint32_t> state_{ kUnlocked };

	static constexpr uint32_t kUnlocked = 0;
	static constexpr uint32_t kLocked = 1;
	static constexpr uint32_t kSleeper = 2;
};

class XAMP_BASE_API SRWMutex {
public:
	SRWMutex() = default;

	XAMP_DISABLE_COPY(SRWMutex)

	void lock() noexcept;
	
	void unlock() noexcept;

	[[nodiscard]] bool try_lock() noexcept;
private:
	SRWLOCK lock_ = SRWLOCK_INIT;
};

class XAMP_BASE_API CriticalSection {
public:
	CriticalSection();

	XAMP_DISABLE_COPY(CriticalSection)

	~CriticalSection() noexcept;

	void lock() noexcept;
	
	void unlock() noexcept;

private:
	CRITICAL_SECTION cs_;
};
	
using FastMutex = SRWMutex;
	
#else
using FastMutex = std::mutex;
#endif
	
}

