//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>
#include <coroutine>
#include <future>
#include <latch>
#include <memory>
#include <stop_token>
#include <type_traits>
#include <tuple>
#include <vector>

#include <base/base.h>
#include <base/fastconditionvariable.h>
#include <base/fastmutex.h>
#include <base/logger.h>
#include <base/memory.h>
#include <base/platform.h>
#include <base/rng.h>
#include <base/stl.h>
#include <base/task.h>
#include <base/workstealingtaskqueue.h>

XAMP_BASE_NAMESPACE_BEGIN

namespace detail {

template <bool WithStopToken, typename F, typename... Args>
struct ThreadPoolSpawnReturn;

template <typename F, typename... Args>
struct ThreadPoolSpawnReturn<true, F, Args...> {
    using Type = std::invoke_result_t<std::decay_t<F>&,
        const std::stop_token&,
        std::decay_t<Args>&...>;
};

template <typename F, typename... Args>
struct ThreadPoolSpawnReturn<false, F, Args...> {
    using Type = std::invoke_result_t<std::decay_t<F>&, std::decay_t<Args>&...>;
};

} // namespace detail

XAMP_MAKE_ENUM(ExecuteFlags, 
    EXECUTE_NORMAL, 
    EXECUTE_LONG_RUNNING)

XAMP_MAKE_ENUM(SubmitPolicy,
    SUBMIT_POLICY_NORMAL,    
    SUBMIT_POLICY_LOCAL,
    SUBMIT_POLICY_FORK)

class XAMP_BASE_API XAMP_NO_VTABLE ITaskScheduler {
public:
    XAMP_BASE_CLASS(ITaskScheduler)

	virtual void submit(Task task, ExecuteFlags flags, SubmitPolicy policty) = 0;

    virtual size_t getThreadSize() const = 0;

    virtual void destroy() = 0;
protected:
    ITaskScheduler() = default;
};

class XAMP_BASE_API XAMP_NO_VTABLE IThreadPool {
public:
    XAMP_BASE_DISABLE_COPY_AND_MOVE(IThreadPool)

    virtual size_t getThreadSize() const = 0;

    virtual void stop() = 0;

    template <typename F, typename... Args>
    decltype(auto) spawn(
        SubmitPolicy policy,
        ExecuteFlags flags,
        F&& f, Args&&... args);

    template <typename F, typename... Args>
    void post(ExecuteFlags flags,
        F&& f,
        Args&&... args);

    void resumeCoroutine(SubmitPolicy policy,
        ExecuteFlags flags,
        std::coroutine_handle<> handle);

    void submitCoroutine(SubmitPolicy policy,
        ExecuteFlags flags,
        Task task);

protected:
    explicit IThreadPool(ScopedPtr<ITaskScheduler> scheduler)
	    : scheduler_(std::move(scheduler)) {
    }

    ScopedPtr<ITaskScheduler> scheduler_;
};

template <typename F, typename ... Args>
void IThreadPool::post(ExecuteFlags flags, F&& f, Args&&... args) {
    scheduler_->submit(
        [func = std::forward<F>(f),
        tuple_args = std::make_tuple(std::forward<Args>(args)...)]
        (const std::stop_token& stop_token) mutable {
            try {
                std::apply([&](auto&... args) {
                    if constexpr (std::is_invocable_v<decltype(func)&, const std::stop_token&, decltype(args)&...>) {
                        std::invoke(func, stop_token, args...);
                    }
                    else {
                        std::invoke(func, args...);
                    }
                    }, tuple_args);
			}
			catch (const std::exception& ex) {
				XAMP_LOG_ERROR("Exception in posted task: {}", ex.what());
			}
            catch (...) {
                XAMP_LOG_ERROR("Unknown exception in posted task.");
            }
        },
        flags,
        SubmitPolicy::SUBMIT_POLICY_LOCAL);
}

template <typename F, typename ... Args>
decltype(auto) IThreadPool::spawn(SubmitPolicy policy, ExecuteFlags flags, F&& f, Args&&... args) {
    constexpr bool kAcceptsStopToken = std::is_invocable_v<std::decay_t<F>&,
        const std::stop_token&,
        std::decay_t<Args>&...>;
    using ReturnType = typename detail::ThreadPoolSpawnReturn<kAcceptsStopToken, F, Args...>::Type;

    using PackagedTaskType = std::packaged_task<ReturnType(const std::stop_token&)>;

    PackagedTaskType task([func = std::forward<F>(f),
                           tuple_args = std::make_tuple(std::forward<Args>(args)...)](
        const std::stop_token& stop_token) mutable -> ReturnType {
        auto invoke_without_stop_token = [&func](auto&... unpacked_args) -> ReturnType {
            if constexpr (std::is_void_v<ReturnType>) {
                std::invoke(func, unpacked_args...);
            }
            else {
                return std::invoke(func, unpacked_args...);
            }
        };

        auto invoke_with_stop_token = [&func, &stop_token](auto&... unpacked_args) -> ReturnType {
            if constexpr (std::is_void_v<ReturnType>) {
                std::invoke(func, stop_token, unpacked_args...);
            }
            else {
                return std::invoke(func, stop_token, unpacked_args...);
            }
        };

        if constexpr (kAcceptsStopToken) {
            return std::apply(invoke_with_stop_token, tuple_args);
        }
        else {
            return std::apply(invoke_without_stop_token, tuple_args);
        }
    });

    auto future = task.get_future();

    scheduler_->submit([t = std::move(task)](const auto& stop_token) mutable {
        t(stop_token);
    }, flags, policy);

    return future;
}

class TaskScheduler final : public ITaskScheduler {
public:
	TaskScheduler(const std::string_view& name,
	              size_t max_thread,
	              size_t bulk_size,
	              ThreadPriority priority);

    XAMP_DISABLE_COPY(TaskScheduler)

    ~TaskScheduler() override;

    size_t getThreadSize() const override;

    void submit(Task task, ExecuteFlags flags, SubmitPolicy policy) override;

    void destroy() override;

private:
    void setWorkerThreadName(size_t i);

    size_t tryDequeueSharedQueue(std::vector<Task>& tasks,
        const std::stop_token& stop_token);

    size_t tryDequeueSharedQueue(std::vector<Task>& tasks,
        const std::stop_token& stop_token,
        std::chrono::milliseconds timeout);

    size_t trySteal(std::vector<Task>& tasks,
        const std::stop_token& stop_token,
        size_t random_start,
        size_t current_thread_index);

    size_t tryLocalPop(std::vector<Task>& tasks,
        const std::stop_token& stop_token,
        WorkStealingTaskQueue* local_queue) const;

    void addThread(size_t i, ThreadPriority priority);

    void execute(std::vector<Task>& tasks,
        size_t task_size,
        size_t current_index,
        const std::stop_token& stop_token);

    void notifyWorkAvailable();

    void notifyAllWorkers();

    void waitForWork(uint32_t observed_epoch,
        const std::stop_token& stop_token);

	bool isLongRunning(size_t index) const;

    template <typename t>
    struct alignas(kCacheAlignSize) AlignedAtomic {
        std::atomic<t> value;
    };

    std::atomic<bool> is_stopped_;    
    std::atomic<size_t> running_thread_;
    size_t max_thread_;
    size_t bulk_size_;
    std::string name_;    
    AlignedAtomic<uint32_t> work_epoch_{0};
    AlignedAtomic<size_t> enqueue_hint_{0};
    SharedTaskQueuePtr shared_queue_;
    std::vector<WorkStealingTaskQueuePtr> task_work_queues_;
    std::vector<std::jthread> threads_;
    std::vector<AlignedAtomic<ExecuteFlags>> task_execute_flags_;
    FastMutex idle_mutex_;
    FastConditionVariable idle_cv_;    
    std::latch work_done_;
    std::latch start_clean_up_;
    LoggerPtr logger_;
};

class ThreadPool final : public IThreadPool {
public:
	explicit ThreadPool(const std::string_view& name,
	                    uint32_t max_thread = std::thread::hardware_concurrency(),
						size_t bulk_size = std::thread::hardware_concurrency() / 2,
	                    ThreadPriority priority = ThreadPriority::PRIORITY_NORMAL);

	~ThreadPool() override;

	XAMP_DISABLE_COPY(ThreadPool)

    void stop() override;

    size_t getThreadSize() const override;
};

XAMP_BASE_NAMESPACE_END
