//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#if 0
#include <array>
#include <string>
#include <sstream>
#include <memory>
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

namespace xamp::base {

using Task = std::function<void()>;

template <typename Type>
class TaskQueue final {
public:
	TaskQueue()
		: done_(false) {
	}

    XAMP_DISABLE_COPY(TaskQueue)   

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
    std::deque<Type> queue_;
};

XAMP_BASE_API void SetCurrentThreadName(int32_t index);

template 
<
	size_t MaxThread,
	typename TaskType, 
	template <typename> 
	class Queue = TaskQueue
>
class TaskScheduler final {
public:
    explicit TaskScheduler()
        : is_stopped_(false)
        , index_(0) {
        for (size_t i = 0; i < MaxThread; ++i) {
			task_queues_[i] = std::move(MakeAlign<TaskQueue<TaskType>>());
        }
        for (size_t i = 0; i < MaxThread; ++i) {
            AddThread(i);
        }
    }

    ~TaskScheduler() {
		Destory();
    }

    XAMP_DISABLE_COPY(TaskScheduler)

    void SubmitJob(TaskType&& task) {
		const auto i = index_++;

        for (size_t n = 0; n != threads_.size(); ++n) {
			const auto index = (i + n) % task_queues_.size();
            if (task_queues_[index]->TryEnqueue(task)) {
                return;
            }
        }
		task_queues_[i % task_queues_.size()]->Enqueue(task);
    }

    void Destory() {
        is_stopped_ = true;

        for (size_t i = 0; i < task_queues_.size(); ++i) {
            task_queues_[i]->Done();

            if (threads_[i].joinable()) {
                threads_[i].join();
            }
        }
    }

private:
    static const int32_t MAX_THREAD_HT_PAD = 64 * 1024;

    void AddThread(size_t i) {
        threads_[i] = std::thread([i, this]() mutable {
			// https://software.intel.com/en-us/articles/multithreaded-game-programming-and-hyper-threading-technology
			const auto cacheline_pad = alloca(((i + 1) % MaxThread) * MAX_THREAD_HT_PAD);

            for (;;) {
                TaskType task;

                for (size_t n = 0; n != MaxThread; ++n) {
                    if (is_stopped_) {
                        return;
                    }

                    const auto index = (i + n) % MaxThread;
                    if (task_queues_[index]->TryDequeue(task)) {
                        break;
                    }
                }

                if (!task) {
                    if (task_queues_[i]->Dequeue(task)) {
                        task();
					} else {
						std::this_thread::yield();
					}
                }
                else {
                    task();
                }
            }
        });
    }

	std::atomic<bool> is_stopped_;
	size_t index_;
    std::array<std::thread, MaxThread> threads_;
	std::array<AlignPtr<Queue<TaskType>>, MaxThread> task_queues_;
};
    
class XAMP_BASE_API ThreadPool final {
public:
	ThreadPool();

	XAMP_DISABLE_COPY(ThreadPool)

    template <typename F, typename... Args>
	std::future<typename std::result_of<F(Args ...)>::type> RunAsync(F&& f, Args&&... args);

private:
	static const size_t MAX_THREAD = 4;
    TaskScheduler<MAX_THREAD, Task> scheduler_;
};

template <typename F, typename ... Args>
std::future<typename std::result_of<F(Args ...)>::type> ThreadPool::RunAsync(F&& f, Args&&... args) {
	typedef typename std::result_of<F(Args ...)>::type ReturnType;

	auto task = std::make_shared<std::packaged_task<ReturnType()>>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
	);

	scheduler_.SubmitJob([task]() {
		(*task)();
	});
	return task->get_future();
}

}

#endif

