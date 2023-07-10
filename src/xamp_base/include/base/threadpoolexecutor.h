//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <future>
#include <atomic>
#include <vector>
#include <optional>
#include <memory>

#include <base/rng.h>
#include <base/jthread.h>
#include <base/base.h>
#include <base/logger.h>
#include <base/align_ptr.h>
#include <base/latch.h>
#include <base/itaskschedulerpolicy.h>
#include <base/ithreadpoolexecutor.h>
#include <base/platform.h>

XAMP_BASE_NAMESPACE_BEGIN

/*
* TaskScheduler is a thread pool executor.
* 
*/
class TaskScheduler final : public ITaskScheduler {
public:
    /*
    * Constructor.
    */
    TaskScheduler(const std::string_view & pool_name,
        size_t max_thread,
        CpuAffinity affinity,
        ThreadPriority priority);

    /*
    * Constructor.
    */
    TaskScheduler(TaskSchedulerPolicy policy,
        TaskStealPolicy steal_policy,
        const std::string_view& pool_name,
        size_t max_thread,
        CpuAffinity affinity,
        ThreadPriority priority);
	
    XAMP_DISABLE_COPY(TaskScheduler)

    /*
    * Destructor.
    */
    ~TaskScheduler() override;

    /*
    * Get thread pool size.
    */
    size_t GetThreadSize() const override;

    /*
    * Submit job to thread pool.
    */
    void SubmitJob(MoveOnlyFunction&& task, ExecuteFlags flags) override;

    /*
    * Destroy thread pool.
    */
    void Destroy() noexcept override;
private:
    /*
    * Set thread name.
    */
    void SetWorkerThreadName(size_t i);
    
    /*
    * Try dequeue task from shared queue.
    */
    std::optional<MoveOnlyFunction> TryDequeueSharedQueue();

    /*
    * Try steal task from other thread.
    */
    std::optional<MoveOnlyFunction> TrySteal(StopToken const & stop_token, size_t i);

    /*
    * Try dequeue task from local queue.
    */
    std::optional<MoveOnlyFunction> TryLocalPop(WorkStealingTaskQueue * local_queue) const;

    /*
    * Try dequeue task from shared queue.
    */
    std::optional<MoveOnlyFunction> TryDequeueSharedQueue(std::chrono::milliseconds timeout);

    /*
    * Add thread to thread pool.
    */
    void AddThread(size_t i, ThreadPriority priority);

    std::atomic<bool> is_stopped_;
    std::atomic<size_t> running_thread_;
    std::atomic<size_t> last_idle_thread_count_;
    size_t max_thread_;
    size_t min_thread_;
    FastMutex mutex_;
    ThreadPriority thread_priority_;
    std::string pool_name_;
    Vector<JThread> threads_;
    Vector<std::atomic<ExecuteFlags>> task_execute_flags_;
    SharedTaskQueuePtr task_pool_;
    AlignPtr<ITaskStealPolicy> task_steal_policy_;
    AlignPtr<ITaskSchedulerPolicy> task_scheduler_policy_;
    Vector<WorkStealingTaskQueuePtr> task_work_queues_;
    Latch work_done_;
    Latch start_clean_up_;
    LoggerPtr logger_;
};

class ThreadPoolExecutor final : public IThreadPoolExecutor {
public:
	ThreadPoolExecutor(const std::string_view& pool_name,
        TaskSchedulerPolicy policy,
        TaskStealPolicy steal_policy,
	    uint32_t max_thread = std::thread::hardware_concurrency(), 
        CpuAffinity affinity = CpuAffinity::kAll,
	    ThreadPriority priority = ThreadPriority::NORMAL);

    explicit ThreadPoolExecutor(const std::string_view& pool_name,
        uint32_t max_thread = std::thread::hardware_concurrency(),
        CpuAffinity affinity = CpuAffinity::kAll,
        ThreadPriority priority = ThreadPriority::NORMAL);

	~ThreadPoolExecutor() override;
    
	XAMP_DISABLE_COPY(ThreadPoolExecutor)

    void Stop() override;

    size_t GetThreadSize() const override;
};

XAMP_BASE_NAMESPACE_END
