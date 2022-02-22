//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ctime>

#include <mutex>
#include <base/windows_handle.h>
#include <base/exception.h>

namespace xamp::base {

#if defined(XAMP_OS_WIN) || defined (XAMP_OS_MAC)

static constexpr uint32_t kUnlocked = 0;
static constexpr uint32_t kLocked = 1;
static constexpr uint32_t kSleeper = 2;

#ifdef XAMP_OS_WIN
class XAMP_BASE_API SRWMutex final {
public:
	SRWMutex() noexcept;

	XAMP_DISABLE_COPY(SRWMutex)

	void lock() noexcept;
	
	void unlock() noexcept;

	[[nodiscard]] bool try_lock() noexcept;

	PSRWLOCK native_handle() {
		return &lock_;
	}
private:
	XAMP_CACHE_ALIGNED(kCacheAlignSize) SRWLOCK lock_;
	uint8_t padding_[kCacheAlignSize - sizeof(lock_)]{ 0 };
};
using FastMutex = SRWMutex;
#else
using FastMutex = std::mutex;
#endif

// note:
// 在MSVC STL C++20實現了std::atomic_wait是以WaitOnAddress實現, 
// 但是在std::condition_variable是以SleepConditionVariableSRW實現.
// FastConditionVariable適用於併發數小的場景上.
// https://www.remlab.net/op/futex-condvar.shtml
class XAMP_BASE_API FastConditionVariable final {
public:
	FastConditionVariable() noexcept = default;

	XAMP_DISABLE_COPY(FastConditionVariable)

	void wait(std::unique_lock<FastMutex>& lock);

	template <typename Predicate>
	void wait(std::unique_lock<FastMutex>& lock, Predicate&& predicate) {
		while (!predicate()) {
			wait(lock);
		}
	}

	template <typename Rep, typename Period>
	std::cv_status wait_for(std::unique_lock<FastMutex>& lock, const std::chrono::duration<Rep, Period>& rel_time) {
		auto old_state = state_.load(std::memory_order_relaxed);
		lock.unlock();
		auto ret = FastWait(state_, old_state, rel_time);
		lock.lock();
		return ret;
	}

	void notify_one() noexcept;

	void notify_all() noexcept;
private:
    static int FastWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, const struct timespec* to) noexcept;

	template <typename Rep, typename Period>
	std::cv_status FastWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, std::chrono::duration<Rep, Period> const& duration) {
		using namespace std::chrono;
		timespec ts;
		ts.tv_sec = duration_cast<seconds>(duration).count();
		ts.tv_nsec = duration_cast<nanoseconds>(duration).count() % 1000000000;
        return FastWait(to_wait_on, expected, &ts) == -1
			? std::cv_status::timeout : std::cv_status::no_timeout;
	}

	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<uint32_t> state_{ kUnlocked };
	uint8_t padding_[kCacheAlignSize - sizeof(state_)]{ 0 };
};
	
#else
using FastMutex = std::mutex;
using FastConditionVariable = std::condition_variable;
#endif
	
}

