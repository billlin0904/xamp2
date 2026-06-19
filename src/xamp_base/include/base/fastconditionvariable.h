//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <condition_variable>
#include <chrono>
#include <mutex>
#include <base/base.h>
#include <base/platform.h>
#include <base/fastmutex.h>

XAMP_BASE_NAMESPACE_BEGIN

#if defined(XAMP_OS_WIN) || defined(XAMP_OS_MAC) || defined(XAMP_OS_LINUX)

/*
* FastConditionVariable is a condition variable that is faster than std::condition_variable.
* 
* See: https://www.remlab.net/op/futex-condvar.shtml
*/
class XAMP_BASE_API FastConditionVariable final {
public:
	FastConditionVariable() = default;

	XAMP_DISABLE_COPY(FastConditionVariable)

	~FastConditionVariable() = default;

	void wait(std::unique_lock<FastMutex>& lock);

	template <typename Predicate>
	void wait(std::unique_lock<FastMutex>& lock, Predicate&& predicate) {
        for (;;) {
            const auto old_state = state_.load(std::memory_order_acquire);
            if (predicate()) {
                return;
            }
            lock.unlock();
            atomicWait(state_, old_state, kInfinity);
            lock.lock();
        }
	}

	template <typename Rep, typename Period>
	std::cv_status wait_for(std::unique_lock<FastMutex>& lock, const std::chrono::duration<Rep, Period>& rel_time) {
        if (rel_time <= std::chrono::duration<Rep, Period>::zero()) {
            return std::cv_status::timeout;
        }

		auto old_state = state_.load(std::memory_order_acquire);
		lock.unlock();
		auto ret = fastWait(state_, old_state, rel_time);
		lock.lock();
		return ret;
	}

    template <typename Rep, typename Period, typename Predicate>
    bool wait_for(std::unique_lock<FastMutex>& lock,
        const std::chrono::duration<Rep, Period>& rel_time,
        Predicate&& predicate) {
        const auto timeout_time = std::chrono::steady_clock::now() + rel_time;

        for (;;) {
            const auto old_state = state_.load(std::memory_order_acquire);
            if (predicate()) {
                return true;
            }

            const auto now = std::chrono::steady_clock::now();
            if (now >= timeout_time) {
                return predicate();
            }

            lock.unlock();
            const auto ret = fastWait(state_, old_state, timeout_time - now);
            lock.lock();
            if (ret == std::cv_status::timeout) {
                return predicate();
            }
        }
    }

	void notify_one() ;

	void notify_all() ;
private:
	template <typename Rep, typename Period>
	std::cv_status fastWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, std::chrono::duration<Rep, Period> const& duration) {
		using namespace std::chrono;		
		timespec ts{};
		ts.tv_sec = duration_cast<seconds>(duration).count();
		ts.tv_nsec = duration_cast<nanoseconds>(duration).count() % 1000000000;		
        return atomicWait(to_wait_on, expected, &ts) == -1 // ABI
			? std::cv_status::timeout : std::cv_status::no_timeout;
	}

    // 這是 futex/WaitOnAddress 用的 sequence counter，不是等待者計數器。
    // notify 時只要讓值不同，等待中的 thread 就會醒來重新檢查 predicate。
    // uint32_t 理論上會 overflow，但只有在同一個 waiter 讀到 old_state 之後，
    // 又剛好發生 2^32 次 notify 並繞回同一個值，才可能造成 ABA lost wake；
    // 同一個 futex 上同時睡眠的 thread 數量不可能接近 2^32，所以這個風險可忽略。
	XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<uint32_t> state_{ 0 };
};

#else
using FastConditionVariable = std::condition_variable_any;
#endif

XAMP_BASE_NAMESPACE_END

