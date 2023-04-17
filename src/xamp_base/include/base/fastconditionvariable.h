//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <mutex>
#include <base/base.h>
#include <base/platform.h>
#include <base/fastmutex.h>

XAMP_BASE_NAMESPACE_BEGIN

#if defined(XAMP_OS_WIN) || defined(XAMP_OS_MAC)

inline constexpr uint32_t kUnlocked = 0;
inline constexpr uint32_t kLocked = 1;
inline constexpr uint32_t kSleeper = 2;

/*
* FastConditionVariable is a condition variable that is faster than std::condition_variable.
* 
* See: https://www.remlab.net/op/futex-condvar.shtml
*/
class XAMP_BASE_API FastConditionVariable final {
public:
	/*
	* Construct.
	* 
	*/
	FastConditionVariable() noexcept = default;

	XAMP_DISABLE_COPY(FastConditionVariable)

	/*
	* Destruct.
	*/
	~FastConditionVariable() noexcept = default;

	/*
	* Wait for the condition variable.
	* 
	* @param lock The lock.	 
	*/
	void wait(std::unique_lock<FastMutex>& lock);

	/*
	* Wait for the condition variable.
	* 
	* @param lock The lock.
	* @param predicate The predicate.
	*/
	template <typename Predicate>
	void wait(std::unique_lock<FastMutex>& lock, Predicate&& predicate) {
		while (!predicate()) {
			wait(lock);
		}
	}

	/*
	* Wait for the condition variable.
	* 
	* @param lock The lock.
	* @param rel_time The relative time.
	* @return std::cv_status::no_timeout if the condition variable is notified, std::cv_status::timeout if the condition variable is timeout.	
	*/
	template <typename Rep, typename Period>
	std::cv_status wait_for(std::unique_lock<FastMutex>& lock, const std::chrono::duration<Rep, Period>& rel_time) {
		// We need to unlock the mutex before we can wait.
		auto old_state = state_.load(std::memory_order_relaxed);
		// If we are the first to try to wait, we need to set the state to kSleeper.
		lock.unlock();
		// If we are the first to try to wait, we need to set the state to kSleeper.
		auto ret = FastWait(state_, old_state, rel_time);
		// We need to lock the mutex again before we can return.
		lock.lock();
		// If we are the first to try to wait, we need to set the state to kSleeper.
		return ret;
	}

	/*
	* Notify one thread.
	* 
	*/
	void notify_one() noexcept;

	/*
	* Notify all threads.
	* 
	*/
	void notify_all() noexcept;
private:
	/*
	* Wait for the condition variable.
	* 
	* @param to_wait_on The atomic variable to wait on.
	* @param expected The expected value.
	* @param duration The duration.
	* @return std::cv_status::no_timeout if the condition variable is notified, std::cv_status::timeout if the condition variable is timeout.
	*/
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

XAMP_BASE_NAMESPACE_END

