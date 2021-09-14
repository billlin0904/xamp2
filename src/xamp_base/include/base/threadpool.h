//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <future>
#include <atomic>
#include <vector>
#include <optional>
#include <type_traits>
#include <memory>

#include <base/base.h>
#include <base/align_ptr.h>
#include <base/bounded_queue.h>

#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
#include <base/logger.h>
#endif

namespace xamp::base {

class TaskWrapper {
public:
    template <typename Func>
    TaskWrapper(Func&& f)
        : impl_(MakeAlign<ImplBase, ImplType<Func>>(std::forward<Func>(f))) {
    }
	
    XAMP_ALWAYS_INLINE void operator()() {
	    impl_->Call();
    }

    XAMP_ALWAYS_INLINE long long ExecutedTime() const noexcept {
        return impl_->ExecutedTime();
    }
	
    TaskWrapper() = default;
	
    TaskWrapper(TaskWrapper&& other) noexcept
		: impl_(std::move(other.impl_)) {	    
    }
	
    TaskWrapper& operator=(TaskWrapper&& other) noexcept {
        impl_ = std::move(other.impl_);
        return *this;
    }
	
    XAMP_DISABLE_COPY(TaskWrapper)
	
private:
    struct XAMP_NO_VTABLE ImplBase {
        virtual ~ImplBase() = default;
        virtual void Call() = 0;
        virtual long long ExecutedTime() const noexcept = 0;
    };

    AlignPtr<ImplBase> impl_;

    template <typename Func>
    struct ImplType final : ImplBase {
	    ImplType(Func&& f)
            : f_(std::forward<Func>(f)) {
        }

#if defined(XAMP_ENABLE_THREAD_POOL_DEBUG) && defined(_DEBUG)
        virtual ~ImplType() noexcept override {
            XAMP_LOG_DEBUG("ImplType was deleted.");
        }
#endif

        XAMP_ALWAYS_INLINE void Call() override {
            f_();
        }

        static std::chrono::time_point<std::chrono::steady_clock> Now() noexcept {
            return std::chrono::steady_clock::now();
	    }

        long long ExecutedTime() const noexcept override {
            const auto end = Now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(end - start_time_).count();
        }

        const std::chrono::time_point<std::chrono::steady_clock> start_time_ = Now();
        Func f_;
    };
};

using Task = TaskWrapper;
	
class XAMP_BASE_API TaskScheduler final {
public:
    explicit TaskScheduler(size_t max_thread, int32_t affinity);
	
    XAMP_DISABLE_COPY(TaskScheduler)

    ~TaskScheduler() noexcept;

    void SubmitJob(Task&& task);

    void SetAffinityMask(int32_t core);

    void Destroy() noexcept;

    size_t GetActiveThreadCount() const noexcept;

private:
    static void SetWorkerThreadName(size_t i);
	
    std::optional<Task> TryPopPoolQueue();

    std::optional<Task> TryPopLocalQueue(size_t index);

    std::optional<Task> TrySteal(size_t i);

    void AddThread(size_t i);

    using TaskQueue = BoundedQueue<Task, FastMutex, FastMutexConditionVariable>;
    using SharedTaskQueuePtr = AlignPtr<TaskQueue>;
    
    static constexpr size_t K = 4;
    static constexpr size_t kInitL1CacheLineSize = 4 * 1024;
    static constexpr size_t kMaxL1CacheLineSize = 64 * 1024;

	std::atomic<bool> is_stopped_;
    std::atomic<size_t> active_thread_;
    int32_t affinity_;
	size_t index_;
    size_t max_thread_;
    TaskQueue pool_queue_;
    std::vector<std::thread> threads_;
    std::vector<SharedTaskQueuePtr> shared_queues_;
    std::shared_ptr<spdlog::logger> logger_;
};

class XAMP_BASE_API ThreadPool final {
public:
    static ThreadPool& StreamReaderThreadPool();

    static ThreadPool& WASAPIThreadPool();
	
    static constexpr uint32_t kMaxThread = 32;
    
	XAMP_DISABLE_COPY(ThreadPool)

    template <typename F, typename... Args>
    decltype(auto) Spawn(F&& f, Args&&... args);

    size_t GetActiveThreadCount() const noexcept;

    void Stop();

    void SetAffinityMask(int32_t core);

private:
	explicit ThreadPool(uint32_t max_thread = std::thread::hardware_concurrency(), int32_t affinity = kDefaultAffinityCpuCore);
    TaskScheduler scheduler_;
};

template <typename F, typename ... Args>
decltype(auto) ThreadPool::Spawn(F &&f, Args&&... args) {	
    using ReturnType = std::invoke_result_t<F, Args...>;

    // MSVC packaged_task can't be constructed from a move-only lambda
    // https://github.com/microsoft/STL/issues/321
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
