//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <condition_variable>
#include <atomic>
#include <queue>
#include <mutex>

#include <base/base.h>
#include <base/circularbuffer.h>
#include <base/fastmutex.h>

namespace xamp::base {

template
<
    typename T,
    typename Mutex = FastMutex,
    typename ConditionVariable = FastConditionVariable,
    typename Queue = CircularBuffer<T>,
    typename V = 
    std::enable_if_t
	<
		std::is_nothrow_move_assignable_v<T>
	>
>
class XAMP_BASE_API_ONLY_EXPORT BlockingQueue final {
public:
	explicit BlockingQueue(size_t size)
		: done_(false)
        , queue_(size) {
	}

    XAMP_DISABLE_COPY(BlockingQueue)   

    template <typename U>
    bool TryEnqueue(U &&task) noexcept {
        {
	        const std::unique_lock lock{mutex_, std::try_to_lock};
            if (!lock) {
                return false;
            }
            queue_.emplace(std::forward<T>(task));
        }
        notify_.notify_one();
        return true;
    }

    template <typename U>
    void Enqueue(U &&task) noexcept {
        {
            std::lock_guard guard{ mutex_ };
            queue_.emplace(std::forward<T>(task));
        }
        notify_.notify_one();
    }

    bool TryDequeue(T& task) {
		const std::unique_lock lock{ mutex_, std::try_to_lock };

		if (!lock || queue_.empty()) {
			return false;
		}

		task = std::move(queue_.pop());
		return true;
	}

	bool Dequeue(T& task) {
		std::unique_lock guard{ mutex_ };

		while (queue_.empty() && !done_) {
			notify_.wait(guard);
		}

		if (queue_.empty()) {
			return false;
		}

		task = std::move(queue_.pop());
		return true;
	}

    bool Dequeue(T& task, const std::chrono::milliseconds wait_time) {
        std::unique_lock guard{ mutex_ };

        // Note: cv.wait_for() does not deal with spurious weak up
        while (queue_.empty() && !done_) {
            if (std::cv_status::timeout == notify_.wait_for(guard, wait_time)) {
                return false;
            }
        }
    
        if (queue_.empty() || done_) {
            return false;
        }

        task = std::move(queue_.pop());
        return true;
    }

    void WakeupForShutdown() noexcept {
        {
            std::lock_guard guard{ mutex_ };
            done_ = true;
        }
        notify_.notify_all();
    }

    bool IsEmpty() const noexcept {
        std::lock_guard guard{ mutex_ };
        return queue_.empty();
    }

    bool IsFull() const noexcept {
        std::lock_guard guard{ mutex_ };
        return queue_.full();
    }

    [[nodiscard]] size_t size() const noexcept {
        std::lock_guard guard{ mutex_ };
        return queue_.size();
    }

private:
    std::atomic<bool> done_;
    mutable Mutex mutex_;
    ConditionVariable notify_;
    Queue queue_;
};

}
