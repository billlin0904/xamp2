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
	TaskScheduler(const std::string_view& name,
	              size_t max_thread,
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

	void SetBulkSize(size_t max_size) override;

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
    size_t TryDequeueSharedQueue(Vector<MoveOnlyFunction>& tasks, const StopToken& stop_token);

    /*
    * Try dequeue task from shared queue.
    */
    size_t TryDequeueSharedQueue(Vector<MoveOnlyFunction>& tasks, const StopToken& stop_token, std::chrono::milliseconds timeout);

    /*
    * Try steal task from other thread.
    */
    size_t TrySteal(Vector<MoveOnlyFunction> &tasks, const StopToken& stop_token, size_t random_start, size_t current_thread_index);

    /*
    * Try dequeue task from local queue.
    */
    size_t TryLocalPop(Vector<MoveOnlyFunction>& tasks, const StopToken& stop_token, WorkStealingTaskQueue* local_queue) const;

    /*
    * Add thread to thread pool.
    */
    void AddThread(size_t i, ThreadPriority priority);

    /*
	 * Execute task.
	 */
    void Execute(Vector<MoveOnlyFunction>& tasks, size_t task_size, size_t current_index, const StopToken& stop_token);

    std::atomic<bool> is_stopped_;
    std::atomic<size_t> running_thread_;
    size_t max_thread_;
	size_t bulk_size_;
    std::string name_;
    Vector<JThread> threads_;
    Vector<std::atomic<ExecuteFlags>> task_execute_flags_;
    SharedTaskQueuePtr task_pool_;
    Vector<WorkStealingTaskQueuePtr> task_work_queues_;    
    Latch work_done_;
    Latch start_clean_up_;
    LoggerPtr logger_;
};

class ThreadPoolExecutor final : public IThreadPoolExecutor {
public:
	explicit ThreadPoolExecutor(const std::string_view& name,
	                            uint32_t max_thread = std::thread::hardware_concurrency(),
	                            ThreadPriority priority = ThreadPriority::PRIORITY_NORMAL);

	~ThreadPoolExecutor() override;
    
	XAMP_DISABLE_COPY(ThreadPoolExecutor)

    void Stop() override;

    size_t GetThreadSize() const override;

	void SetBulkSize(size_t max_size) override;
};

XAMP_BASE_NAMESPACE_END
