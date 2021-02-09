//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <condition_variable>
#include <atomic>
#include <queue>
#include <mutex>

#include <base/base.h>
#include <base/circularbuffer.h>

namespace xamp::base {

template <typename Type>
class XAMP_BASE_API_ONLY_EXPORT BoundedQueue final {
public:
	explicit BoundedQueue(size_t size)
		: done_(false)
        , queue_(size) {
	}

    XAMP_DISABLE_COPY(BoundedQueue)   

    template <typename U>
    bool TryEnqueue(U &&task) {
        {
	        const std::unique_lock<std::mutex> lock{mutex_, std::try_to_lock};
            if (!lock) {
                return false;
            }
            queue_.emplace(std::forward<U>(task));
        }
        notify_.notify_one();
        return true;
    }

    template <typename U>
    void Enqueue(U &&task) {        
        {
            std::lock_guard guard{ mutex_ };
            queue_.emplace(std::move(task));
        }
        notify_.notify_one();
    }

	bool TryDequeue(Type& task) {
		const std::unique_lock<std::mutex> lock{ mutex_, std::try_to_lock };

		if (!lock || queue_.empty()) {
			return false;
		}

		task = std::move(queue_.pop());
		return true;
	}

	bool Dequeue(Type& task) {
		std::unique_lock<std::mutex> guard{ mutex_ };

		while (queue_.empty() && !done_) {
			notify_.wait(guard);
		}

		if (queue_.empty()) {
			return false;
		}

		task = std::move(queue_.pop());
		return true;
	}

    bool Dequeue(Type& task, const std::chrono::milliseconds wait_time) {
        std::unique_lock<std::mutex> guard{mutex_};

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

    void WakeupForShutdown() {
        {
            std::lock_guard<std::mutex> guard{ mutex_ };
            done_ = true;
        }
        notify_.notify_all();
    }

private:
    std::atomic<bool> done_;
    mutable std::mutex mutex_;
    std::condition_variable notify_;
    CircularBuffer<Type> queue_;
};

}
