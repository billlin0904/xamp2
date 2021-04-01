//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#ifdef XAMP_OS_WIN

#include <mutex>
#include <base/windows_handle.h>
#include <output_device/win32/hrexception.h>

namespace xamp::output_device::win32 {

// https://probablydance.com/2019/12/30/measuring-mutexes-spinlocks-and-how-bad-the-linux-scheduler-really-is/
struct XAMP_CACHE_ALIGNED(kMallocAlignSize) SpinLock {
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
typedef SpinLock FastMutex;
	
}
#endif
