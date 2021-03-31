//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#ifdef XAMP_OS_WIN

#include <base/windows_handle.h>
#include <output_device/win32/hrexception.h>

namespace xamp::output_device::win32 {

struct SpinLock {
public:
	XAMP_DISABLE_COPY(SpinLock)
		
	void Lock() noexcept {
		for (;;) {
			if (!lock_.exchange(true, std::memory_order_acquire)) {
				return;
			}
			while (lock_.load(std::memory_order_relaxed)) {
				// Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
				// hyper-threads
				//__builtin_ia32_pause();
				YieldProcessor();
			}
		}
	}
	
	bool TryLock() noexcept {
		return !lock_.load(std::memory_order_relaxed) &&
			!lock_.exchange(true, std::memory_order_acquire);
	}

	void Unlock() noexcept {
		lock_.store(false, std::memory_order_release);
	}
	
private:
	std::atomic<bool> lock_ = { false };
};

class FastMutex {
public:
	FastMutex() {
		if (!::InitializeCriticalSectionEx(&cs_, 1, CRITICAL_SECTION_NO_DEBUG_INFO)) {
			throw PlatformSpecException();
		}
		::SetCriticalSectionSpinCount(&cs_, 0);
	}

	XAMP_DISABLE_COPY(FastMutex)

	~FastMutex() noexcept {
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

}
#endif
