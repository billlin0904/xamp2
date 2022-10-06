//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <mutex>
#include <base/base.h>
#include <base/platform.h>
#include <base/fastmutex.h>

namespace xamp::base {

#if defined(XAMP_OS_WIN) || defined(XAMP_OS_MAC)

inline constexpr uint32_t kUnlocked = 0;
inline constexpr uint32_t kLocked = 1;
inline constexpr uint32_t kSleeper = 2;

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
	template <typename Rep, typename Period>
	std::cv_status FastWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, std::chrono::duration<Rep, Period> const& duration) {
		using namespace std::chrono;
		timespec ts{};
		ts.tv_sec = duration_cast<seconds>(duration).count();
		ts.tv_nsec = duration_cast<nanoseconds>(duration).count() % 1000000000;
        return AtomicWait(to_wait_on, expected, &ts) == -1 // ABI
			? std::cv_status::timeout : std::cv_status::no_timeout;
	}

	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<uint32_t> state_{ kUnlocked };
	uint8_t padding_[kCacheAlignSize - sizeof(state_)]{ 0 };
};

#else
using FastConditionVariable = std::condition_variable;
#endif

}

