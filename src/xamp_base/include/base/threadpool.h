//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <future>
#include <atomic>
#include <queue>
#include <mutex>
#include <functional>
#include <vector>
#include <condition_variable>
#include <type_traits>

#include <base/base.h>
#include <base/memory.h>
#include <base/align_ptr.h>
#include <base/stl.h>
#include <base/circularbuffer.h>
#include <base/platform_thread.h>

namespace xamp::base {

template <typename Type>
class BoundedQueue final {
public:
	BoundedQueue(size_t const size)
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
            queue_.push(std::move(task));
        }
        notify_.notify_one();
        return true;
    }

    template <typename U>
    void Enqueue(U &&task) {
        std::unique_lock<std::mutex> guard{mutex_};
        queue_.push(std::move(task));
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

    bool Dequeue(Type& task, std::chrono::milliseconds wait_time) {
        std::unique_lock<std::mutex> guard{mutex_};

        // Note: cv.wait_for() does not deal with spurious wakeups
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

    void Destory() {
        {
            std::unique_lock<std::mutex> guard{ mutex_ };
            done_ = true;
        }
        notify_.notify_all();
    }

private:
    std::atomic<bool> done_;
    mutable std::mutex mutex_;
    std::condition_variable notify_;
    circular_buffer<Type> queue_;
};

template 
<
	typename TaskType, 
	template <typename> 
	class Queue = BoundedQueue
>
class TaskScheduler final {
public:
    static constexpr size_t K = 4;

    explicit TaskScheduler(size_t max_thread)
        : is_stopped_(false)
        , active_thread_(0)
        , index_(0)
        , max_thread_(max_thread) {
        for (size_t i = 0; i < max_thread_; ++i) {
            task_queues_.push_back(MakeAlign<BoundedQueue<TaskType>>(max_thread));
        }
        for (size_t i = 0; i < max_thread_; ++i) {
            AddThread(i);
        }
    }

    ~TaskScheduler() {
		Destory();
    }

    XAMP_DISABLE_COPY(TaskScheduler)

    void SubmitJob(TaskType&& task) {
		const auto i = index_++;
        for (size_t n = 0; n < max_thread_ * K; ++n) {
			const auto index = (i + n) % max_thread_;
            if (task_queues_[index]->TryEnqueue(std::move(task))) {
                return;
            }
        }
		task_queues_[i % max_thread_]->Enqueue(std::move(task));
    }

    void Destory() {
        is_stopped_ = true;

        for (size_t i = 0; i < max_thread_; ++i) {
            task_queues_[i]->Destory();

            if (threads_[i].joinable()) {
                threads_[i].join();
            }
        }
    }

    size_t GetActiveThreadCount() const {
        return active_thread_;
    }

private:
    void AddThread(size_t i) {
        threads_.push_back(std::thread([i, this]() mutable {
            SetCurrentThreadName(i);

            for (;;) {
                TaskType task;

                for (size_t n = 0; n != max_thread_; ++n) {
                    if (is_stopped_) {
                        return;
                    }

                    const auto index = (i + n) % max_thread_;
                    if (task_queues_[index]->TryDequeue(task)) {
                        break;
                    }
                }

                if (!task) {
                    if (task_queues_[i]->Dequeue(task)) {
                        ++active_thread_;
                        task();
                        --active_thread_;
					} else {
						std::this_thread::yield();
					}
                }
                else {
                    ++active_thread_;
                    task();
                    --active_thread_;
                }
            }
        }));
    }

    using TaskQueuePtr = align_ptr<Queue<TaskType>>;   

	std::atomic<bool> is_stopped_;
    std::atomic<size_t> active_thread_;
	size_t index_;
    size_t max_thread_;
    std::vector<std::thread> threads_;
    std::vector<TaskQueuePtr> task_queues_;
};

class XAMP_BASE_API ThreadPool final {
public:
    static constexpr uint32_t MAX_THREAD = 8;

    ThreadPool();

    static ThreadPool& DefaultThreadPool();

	XAMP_DISABLE_COPY(ThreadPool)

    template <typename F, typename... Args>
    decltype(auto) StartNew(F f, Args&&... args);

    size_t GetActiveThreadCount() const;

private:
    using Task = std::function<void()>;
    TaskScheduler<Task> scheduler_;
};

template <typename F, typename ... Args>
decltype(auto) ThreadPool::StartNew(F f, Args&&... args) {
    using ReturnType = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    auto future = task->get_future();

	scheduler_.SubmitJob([task]() mutable {
        (*task)();
	});

    return future.share();
}

}
