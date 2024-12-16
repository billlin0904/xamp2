//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <condition_variable>
#include <atomic>
#include <queue>
#include <mutex>

#include <base/base.h>
#include <base/circularbuffer.h>
#include <base/fastconditionvariable.h>
#include <base/fastmutex.h>

XAMP_BASE_NAMESPACE_BEGIN

template
<
    typename T,
    typename Mutex = FastMutex,
    typename Queue = CircularBuffer<T>,
    typename ConditionVariable = FastConditionVariable,
    typename V = 
    std::enable_if_t
	<
		std::is_nothrow_move_assignable_v<T>
	>
>
class XAMP_BASE_API_ONLY_EXPORT BlockingQueue final {
public:
    /*
    * Constructor.
    */
	explicit BlockingQueue(size_t size)
		: done_(false)
        , queue_(size) {
	}

    XAMP_DISABLE_COPY(BlockingQueue)   

    /*
    * Destructor.    
    */
    ~BlockingQueue() {
		wakeup_for_shutdown();
	}

    template <typename U>
    bool try_dequeue(U &&task) noexcept {
        {
	        const std::unique_lock lock{mutex_, std::try_to_lock};
            if (!lock) {
                return false;
            }
            if (queue_.full()) {
				return false;
			}
            queue_.emplace(std::forward<T>(task));
        } // unlock
        notify_.notify_one();
        return true;
    }

    /*
    * enqueue a task to the queue.
    * 
    * @param[in] task The task to enqueue.
    */
    template <typename U>
    void enqueue(U &&task) {
        {
            std::lock_guard guard{ mutex_ };
            queue_.emplace(std::forward<T>(task));
        } // unlock
        notify_.notify_one();
    }

    /*
    * Pops a value from the queue.
    * 
    * @param[in] value The value popped from the queue.
    * @return true if the value was popped, false if the queue is empty.
    */
    bool try_dequeue(T& value) {
        const std::unique_lock lock{ mutex_, std::try_to_lock };
        if (!lock) {
            return false;
        }

        if (queue_.empty()) {
            return false;
        }

        value = std::move(queue_.front());
        queue_.pop();
        return true;
	}

    /*
    * Pops a value from the queue. and wait until the queue is not empty.
    * 
    * @param[in] value The value popped from the queue.
    * @return true if the value was popped, false if the queue is empty.
    */
	bool dequeue(T& task) {
		std::unique_lock guard{ mutex_ };

		while (queue_.empty() && !done_) { 
            // check done_ to avoid spurious wake up            
			notify_.wait(guard);
		}

        if (queue_.empty()) {
			return false;
		}

		task = std::move(queue_.front());
		queue_.pop();
        return true;
	}

    /*
    * Pops a value from the queue. and wait until the queue is not empty.
    *
    * @param[in] value The value popped from the queue.
    * @param[in] wait_time The time to wait for the queue to be not empty.
    * @return true if the value was popped, false if the queue is empty.
    */
    bool dequeue(T& task, const std::chrono::milliseconds wait_time) {
        std::unique_lock guard{ mutex_ };

        while (queue_.empty() && !done_) {
            if (std::cv_status::timeout == notify_.wait_for(guard, wait_time)) {
                return false; // timeout
            }
        }
    
        if (queue_.empty()) { 
            return false;
        }

        task = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    /*
    * Weak up all threads waiting for the queue to be not empty.
    * 
    * This function is used to wake up all threads waiting for the queue to be
    * not empty. This is used to shut down the queue.
    * 
    */
    void wakeup_for_shutdown() {
        {
            std::lock_guard guard{ mutex_ };
            done_ = true;
        }
        notify_.notify_all();
    }

    /*
    * Checks if the queue is empty.
    * 
    * @return true if the queue is empty, false otherwise.    
    */
    bool is_empty() const noexcept {
        std::lock_guard guard{ mutex_ };
        return queue_.empty();
    }

    /*
    * Checks if the queue is full.
    * 
    * @return true if the queue is full, false otherwise.
    */
    bool is_full() const noexcept {
        std::lock_guard guard{ mutex_ };
        return queue_.full();
    }

    /*
    * Get queue size.
    * 
    * @return The queue size.
    */
    XAMP_NO_DISCARD size_t size() const noexcept {
        std::lock_guard guard{ mutex_ };
        return queue_.size();
    }

    void wakeup() {
        notify_.notify_all();
    }

private:
    std::atomic<bool> done_;
    mutable Mutex mutex_;
    ConditionVariable notify_;
    Queue queue_;
};

XAMP_BASE_NAMESPACE_END
