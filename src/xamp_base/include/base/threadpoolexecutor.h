//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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
#include <base/memory.h>
#include <base/latch.h>
#include <base/workstealingtaskqueue.h>
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
	TaskScheduler(const std::string_view& pool_name,
	              size_t max_thread,
	              const CpuAffinity& affinity,
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
    std::optional<MoveOnlyFunction> TryDequeueSharedQueue(const StopToken& stop_token);

    /*
    * Try steal task from other thread.
    */
    std::optional<MoveOnlyFunction> TrySteal(const StopToken& stop_token, size_t current_thread_index);

    /*
    * Try dequeue task from local queue.
    */
    std::optional<MoveOnlyFunction> TryLocalPop(const StopToken& stop_token, WorkStealingTaskQueue* local_queue) const;

    /*
    * Try dequeue task from shared queue.
    */
    std::optional<MoveOnlyFunction> TryDequeueSharedQueue(const StopToken& stop_token, std::chrono::milliseconds timeout);

    /*
    * Add thread to thread pool.
    */
    void AddThread(size_t i, ThreadPriority priority);

    std::atomic<bool> is_stopped_;
    std::atomic<size_t> running_thread_;
    size_t max_thread_;
    FastMutex mutex_;
    std::string pool_name_;
    Vector<JThread> threads_;
    Vector<std::atomic<ExecuteFlags>> task_execute_flags_;
    SharedTaskQueuePtr task_pool_;
    Vector<WorkStealingTaskQueuePtr> task_work_queues_;    
    Latch work_done_;
    Latch start_clean_up_;
    CpuAffinity cpu_affinity_;
    LoggerPtr logger_;
};

class ThreadPoolExecutor final : public IThreadPoolExecutor {
public:
	explicit ThreadPoolExecutor(const std::string_view& pool_name,
	                            uint32_t max_thread = std::thread::hardware_concurrency(),
	                            const CpuAffinity &affinity = CpuAffinity::kAll,
	                            ThreadPriority priority = ThreadPriority::PRIORITY_NORMAL);

	~ThreadPoolExecutor() override;
    
	XAMP_DISABLE_COPY(ThreadPoolExecutor)

    void Stop() override;

    size_t GetThreadSize() const override;
};

XAMP_BASE_NAMESPACE_END
