//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <time.h>

#include <mutex>
#include <base/windows_handle.h>
#include <base/exception.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN

template <typename Rep, typename Period>
constexpr timespec ToTimespec(std::chrono::duration<Rep, Period> const & duration) noexcept {
	using namespace std::chrono;
	timespec ts;
	ts.tv_sec = duration_cast<seconds>(duration).count();
	ts.tv_nsec = duration_cast<nanoseconds>(duration).count() % 1000000000;
	return ts;
}

XAMP_BASE_API int FutexWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, const struct timespec* to);

template <typename Rep, typename Period>
int FutexWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, std::chrono::duration<Rep, Period> const& duration) {
	auto to = ToTimespec(duration);
	return FutexWait(to_wait_on, expected, &to);
}

// https://probablydance.com/2019/12/30/measuring-mutexes-spinlocks-and-how-bad-the-linux-scheduler-really-is/
class XAMP_BASE_API XAMP_CACHE_ALIGNED(kMallocAlignSize) SpinLock final {
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

static constexpr uint32_t kUnlocked = 0;
static constexpr uint32_t kLocked = 1;
static constexpr uint32_t kSleeper = 2;

class XAMP_BASE_API FutexMutex final {
public:
	FutexMutex() = default;

	XAMP_DISABLE_COPY(FutexMutex)

	void lock() noexcept;
	
	void unlock() noexcept;

	bool try_lock() noexcept {
		return state_.exchange(kLocked) == kUnlocked;
	}

private:
	std::atomic<uint32_t> state_{ kUnlocked };	
};

class XAMP_BASE_API SRWMutex final {
public:
	SRWMutex() = default;

	XAMP_DISABLE_COPY(SRWMutex)

	void lock() noexcept;
	
	void unlock() noexcept;

	[[nodiscard]] bool try_lock() noexcept;
private:
	SRWLOCK lock_ = SRWLOCK_INIT;
};

class XAMP_BASE_API CriticalSection final {
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

class XAMP_BASE_API FutexMutexConditionVariable final {
public:
	FutexMutexConditionVariable() = default;

	XAMP_DISABLE_COPY(FutexMutexConditionVariable)

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
		auto ret = FutexWait(state_, old_state, rel_time) == -1
			? std::cv_status::timeout : std::cv_status::no_timeout;
		lock.lock();
		return ret;
	}

	void notify_one() noexcept;

	void notify_all() noexcept;
private:
	std::atomic<uint32_t> state_{ kUnlocked };
};
	
#else
using FastMutex = std::mutex;
#endif
	
}

