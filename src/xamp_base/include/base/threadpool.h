//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
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
#include <type_traits>
#include <memory>

#include <base/base.h>
#include <base/memory.h>
#include <base/singleton.h>
#include <base/align_ptr.h>
#include <base/stl.h>
#include <base/exception.h>
#include <base/bounded_queue.h>
#include <base/platform_thread.h>

#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
#include <base/logger.h>
#endif

namespace xamp::base {

class TaskWrapper {
public:
    template <typename F>
    TaskWrapper(F&& f)
        : impl_(MakeAlign<ImplBase, ImplType<F>>(std::move(f))) {
    }
	
    void operator()() {
	    impl_->Call();
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
    };

    AlignPtr<ImplBase> impl_;

    template <typename F>
    struct ImplType final : ImplBase {
	    ImplType(F&& f)
            : f_(std::move(f)) {
        }

#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
        virtual ~ImplType() noexcept override {
            XAMP_LOG_DEBUG("ImplType was deleted.");
        }
#endif

        void Call() override {
            f_();
        }

        F f_;
    };
};

using Task = TaskWrapper;
	
class XAMP_BASE_API TaskScheduler final {
public:
    explicit TaskScheduler(size_t max_thread, int32_t core = -1);
	
    XAMP_DISABLE_COPY(TaskScheduler)

    ~TaskScheduler() noexcept;

    void SubmitJob(Task&& task);

    void SetAffinityMask(int32_t core);

    void Destroy() noexcept;

    size_t GetActiveThreadCount() const noexcept;

private:
    static void SetWorkerThreadName(int32_t i);
	
    std::optional<Task> TryPopPoolQueue();

    std::optional<Task> TryPopLocalQueue(size_t index);

    std::optional<Task> TrySteal();

    void AddThread(size_t i);

    using TaskQueue = BoundedQueue<Task>;
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
    friend class Singleton<ThreadPool>;
	
    static constexpr uint32_t kMaxThread = 32;
    
	XAMP_DISABLE_COPY(ThreadPool)

    template <typename F, typename... Args>
    decltype(auto) Run(F&& f, Args&&... args);

    size_t GetActiveThreadCount() const noexcept;

    void Stop();

    static ThreadPool& GetInstance();

    void SetAffinityMask(int32_t core);

private:
	ThreadPool();	
    TaskScheduler scheduler_;
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
