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

#include <base/logger.h>
#include <base/base.h>
#include <base/memory.h>
#include <base/align_ptr.h>
#include <base/alignstl.h>

namespace xamp::base {

template <typename Type>
class DispatchQueue final {
public:
	DispatchQueue()
		: done_(false) {
	}

    XAMP_DISABLE_COPY(DispatchQueue)   

    template <typename U>
    bool TryEnqueue(U &&task) {
        {
	        const std::unique_lock<std::mutex> lock{mutex_, std::try_to_lock};
            if (!lock) {
                return false;
            }
            queue_.push_back(std::forward<U>(task));
        }
        notify_.notify_one();
        return true;
    }

    template <typename U>
    void Enqueue(U &&task) {
        std::unique_lock<std::mutex> guard{mutex_};
        queue_.push_back(std::forward<U>(task));
        notify_.notify_one();
    }

	bool TryDequeue(Type& task) {
		const std::unique_lock<std::mutex> lock{ mutex_, std::try_to_lock };

		if (!lock || queue_.empty()) {
			return false;
		}

		task = std::move(queue_.front());
		queue_.pop_front();
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

		task = std::move(queue_.front());
		queue_.pop_front();
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

        task = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    void Done() {
        std::unique_lock<std::mutex> guard{mutex_};
        done_ = true;
        notify_.notify_all();
    }

private:
    std::atomic<bool> done_;
    mutable std::mutex mutex_;
    std::condition_variable notify_;
    Queue<Type> queue_;
};

XAMP_BASE_API void SetCurrentThreadName(size_t index);

template 
<
	typename TaskType, 
	template <typename> 
	class Queue = DispatchQueue
>
class TaskScheduler final {
public:
    explicit TaskScheduler()
        : is_stopped_(false)
        , active_thread_(0)
        , index_(0)
        , max_thread_(std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < max_thread_; ++i) {
            task_queues_.push_back(MakeAlign<DispatchQueue<TaskType>>());
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
        for (size_t n = 0; n < max_thread_; ++n) {
			const auto index = (i + n) % max_thread_;
            if (task_queues_[index]->TryEnqueue(task)) {
                return;
            }
        }
		task_queues_[i % max_thread_]->Enqueue(task);
    }

    void Destory() {
        is_stopped_ = true;

        for (size_t i = 0; i < max_thread_; ++i) {
            task_queues_[i]->Done();

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

                XAMP_LOG_DEBUG("Active thread: {}", i);

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

    using TaskQueuePtr = AlignPtr<Queue<TaskType>>;

	std::atomic<bool> is_stopped_;
    std::atomic<size_t> active_thread_;
	size_t index_;
    size_t max_thread_;
    Vector<std::thread> threads_;
    Vector<TaskQueuePtr> task_queues_;
};
    
class XAMP_BASE_API ThreadPool final {
public:
    ThreadPool();

	XAMP_DISABLE_COPY(ThreadPool)

    template <typename F, typename... Args>
	std::future<typename std::invoke_result<F(Args ...)>::type> RunAsync(F&& f, Args&&... args);

    size_t GetActiveThreadCount() const {
        return scheduler_.GetActiveThreadCount();
    }

private:
    using Task = std::function<void()>;
    TaskScheduler<Task> scheduler_;
};

template <typename F, typename ... Args>
std::future<typename std::invoke_result<F(Args ...)>::type> ThreadPool::RunAsync(F&& f, Args&&... args) {
	typedef typename std::invoke_result<F(Args ...)>::type ReturnType;

	auto task = std::make_shared<std::packaged_task<ReturnType()>>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
	);

	scheduler_.SubmitJob([task]() {
		(*task)();
	});
	return task->get_future();
}

namespace DefaultThreadPool {

XAMP_BASE_API ThreadPool& GetThreadPool();

template <typename Func, typename... Args>
XAMP_BASE_API_ONLY_EXPORT auto RunAsync(Func&& func, Args&&... args) {
    return GetThreadPool().RunAsync(std::forward<Func>(func), std::forward<Args>(args)...);
}

}

}
