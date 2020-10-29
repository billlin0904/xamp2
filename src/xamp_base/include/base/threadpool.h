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
#include <optional>
#include <condition_variable>
#include <type_traits>

#include <base/base.h>
#include <base/memory.h>
#include <base/singleton.h>
#include <base/align_ptr.h>
#include <base/logger.h>
#include <base/stl.h>
#include <base/circularbuffer.h>
#include <base/platform_thread.h>

namespace xamp::base {

class FunctionWrapper {
    struct ImplBase {        
        virtual ~ImplBase() = default;
    	virtual void Call() = 0;
    };
	
    AlignPtr<ImplBase> impl_;
	
    template <typename F>
    struct ImplType final : ImplBase {        
        ImplType(F&& f)
    		: f_(std::move(f)) {	        
        }

        virtual ~ImplType() noexcept override {
            XAMP_LOG_DEBUG("ImplType was deleted.");
        }

        void Call() override {
	        f_();
        }

    	F f_;
    };
	
public:
    template <typename F>
    FunctionWrapper(F&& f)
        : impl_(MakeAlign<ImplBase, ImplType<F>>(std::move(f))) {
    }
	
    void operator()() {
	    impl_->Call();
    }
	
    FunctionWrapper() = default;
	
    FunctionWrapper(FunctionWrapper&& other) noexcept
		: impl_(std::move(other.impl_)) {	    
    }
	
    FunctionWrapper& operator=(FunctionWrapper&& other) noexcept {
        impl_ = std::move(other.impl_);
        return *this;
    }
	
    XAMP_DISABLE_COPY(FunctionWrapper)
};

template <typename Type>
class BoundedQueue final {
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
            queue_.emplace(std::move(task));
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

template 
<
	typename TaskType, 
	template <typename> 
	class Queue = BoundedQueue
>
class TaskScheduler final {
public:
    explicit TaskScheduler(size_t max_thread, int32_t core = 1)
        : is_stopped_(false)
        , active_thread_(0)
        , core_(core)
        , index_(0)
        , max_thread_(max_thread)
        , pool_queue_(max_thread) {
    	try {
    	    for (size_t i = 0; i < max_thread_; ++i) {
                shared_queues_.push_back(MakeAlign<TaskQueue>(max_thread));
            }
            for (size_t i = 0; i < max_thread_; ++i) {
                AddThread(static_cast<int32_t>(i));
            }	
    	} catch (...) {
    		is_stopped_ = true;
    		throw;
    	}
        XAMP_LOG_DEBUG("TaskScheduler initial max thread:{} affinity:{}", max_thread, core);
    }    

    ~TaskScheduler() noexcept {
		Destroy();
    }

    void SubmitJob(TaskType&& task) {
        const auto i = index_++;
        for (size_t n = 0; n < max_thread_ * K; ++n) {
			const auto index = (i + n) % max_thread_;
            if (shared_queues_.at(index)->TryEnqueue(std::move(task))) {
                XAMP_LOG_DEBUG("Enqueue thread {} queue.", index);
                return;
            }
        }
        pool_queue_.Enqueue(std::move(task));
        XAMP_LOG_DEBUG("Enqueue pool queue.");
    }

    void SetAffinityMask(int32_t core) {
        for (size_t i = 0; i < max_thread_; ++i) {
            SetThreadAffinity(threads_.at(i), core);
        }
        core_ = core;
    }

    void Destroy() noexcept {
        is_stopped_ = true;

        for (size_t i = 0; i < max_thread_; ++i) {
            try {
                shared_queues_.at(i)->WakeupForShutdown();

                if (threads_.at(i).joinable()) {
                    threads_.at(i).join();
                }
            }
            catch (...) {
            }
        }

        XAMP_LOG_DEBUG("Thread pool was destory.");
    }

    size_t GetActiveThreadCount() const noexcept {
        return active_thread_;
    }

private:
    std::optional<TaskType> TryPopPoolQueue() {
        constexpr auto kWaitTimeout = std::chrono::milliseconds(30);
        TaskType task;
        if (pool_queue_.Dequeue(task, kWaitTimeout)) {
            XAMP_LOG_DEBUG("Pop pool thread queue.");
            return std::move(task);
        }
        return std::nullopt;
    }

    std::optional<TaskType> TryPopLocalQueue(size_t index) {
        TaskType task;
        if (shared_queues_.at(index)->TryDequeue(task)) {
            XAMP_LOG_DEBUG("Pop local thread queue.");
            return std::move(task);
        }
        return std::nullopt;
    }

    std::optional<TaskType> TrySteal() {
        TaskType task;
        const auto i = 0;
        for (size_t n = 0; n != max_thread_ * K; ++n) {
            if (is_stopped_) {
                return std::nullopt;
            }

            const auto index = (i + n) % max_thread_;
            if (shared_queues_.at(index)->TryDequeue(task)) {
                XAMP_LOG_DEBUG("Steal other thread queue.");
                return std::move(task);
            }
        }
        return std::nullopt;
    }

    void AddThread(size_t i) {
        threads_.push_back(std::thread([i, this]() mutable {
            const auto L1_padding_buffer = MakeStackBuffer<uint8_t>(
                (std::min)(kInitL1CacheLineSize * i, kMaxL1CacheLineSize));
#ifdef XAMP_OS_MAC
            // Sleep for set thread name.
            std::this_thread::sleep_for(std::chrono::milliseconds(900));
#endif
            SetCurrentThreadName(i);

            XAMP_LOG_DEBUG("Thread {} start.", i);

            for (;!is_stopped_;) {                
                auto task = TrySteal();                
                if (!task) {
                    task = TryPopLocalQueue(i);
                }
                if (!task) {
                    std::this_thread::yield();
                    task = TryPopPoolQueue();
                }
                if (!task) {
                    std::this_thread::yield();
                    continue;
                }

                auto active_thread = ++active_thread_;
                XAMP_LOG_DEBUG("Thread {} weakup, active:{}.", i, active_thread);
                (*task)();
                --active_thread_;
                XAMP_LOG_DEBUG("Thread {} execute finished.", i);
            }

            XAMP_LOG_DEBUG("Thread {} done.", i);
        }));

        SetThreadAffinity(threads_.at(i), core_);
    }

    using TaskQueue = Queue<TaskType>;
    using SharedTaskQueuePtr = AlignPtr<TaskQueue>;
    
    static constexpr size_t K = 4;
    static constexpr size_t kInitL1CacheLineSize = 4 * 1024;
    static constexpr size_t kMaxL1CacheLineSize = 64 * 1024;

	std::atomic<bool> is_stopped_;
    std::atomic<size_t> active_thread_;
    int32_t core_;
	size_t index_;
    size_t max_thread_;
    TaskQueue pool_queue_;
    std::vector<std::thread> threads_;
    std::vector<SharedTaskQueuePtr> shared_queues_;
};

class XAMP_BASE_API ThreadPool final {
public:
    static constexpr uint32_t kMaxThread = 8;
    
	XAMP_DISABLE_COPY(ThreadPool)

    template <typename F, typename... Args>
    decltype(auto) Run(F&& f, Args&&... args);

    size_t GetActiveThreadCount() const noexcept;

    void Stop();

    static ThreadPool& Default();

    void SetAffinityMask(int32_t core);

private:
	ThreadPool();
	
	using Task = FunctionWrapper;
    TaskScheduler<Task> scheduler_;
};

template <typename F, typename ... Args>
decltype(auto) ThreadPool::Run(F &&f, Args&&... args) {	
    using ReturnType = std::invoke_result_t<F, Args...>;
	using PackagedTaskType = std::packaged_task<ReturnType()>;

    auto task = std::make_shared<PackagedTaskType>(
        [
            Func = std::forward<F>(f),
            Args = std::make_tuple(std::forward<Args>(args)...)
        ] {
            return std::apply(Func, Args);
	    }
    );

    auto future = task->get_future();

	scheduler_.SubmitJob([task]() mutable {
        (*task)();
	});

    return future.share();
}

}
