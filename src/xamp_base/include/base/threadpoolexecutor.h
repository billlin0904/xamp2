//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <future>
#include <atomic>
#include <vector>
#include <optional>
#include <memory>
#include <latch>

#include <base/rng.h>
#include <base/base.h>
#include <base/logger.h>
#include <base/memory.h>
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
				  size_t bulk_size,
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
    size_t TryDequeueSharedQueue(Vector<MoveOnlyFunction>& tasks, const std::stop_token& stop_token);

    /*
    * Try dequeue task from shared queue.
    */
    size_t TryDequeueSharedQueue(Vector<MoveOnlyFunction>& tasks, const std::stop_token& stop_token, std::chrono::milliseconds timeout);

    /*
    * Try steal task from other thread.
    */
    size_t TrySteal(Vector<MoveOnlyFunction> &tasks, const std::stop_token& stop_token, size_t random_start, size_t current_thread_index);

    /*
    * Try dequeue task from local queue.
    */
    size_t TryLocalPop(Vector<MoveOnlyFunction>& tasks, const std::stop_token& stop_token, WorkStealingTaskQueue* local_queue) const;

    /*
    * Add thread to thread pool.
    */
    void AddThread(size_t i, ThreadPriority priority);

    /*
	 * Execute task.
	 */
    void Execute(Vector<MoveOnlyFunction>& tasks, size_t task_size, size_t current_index, const std::stop_token& stop_token);

    std::atomic<bool> is_stopped_;
    std::atomic<size_t> running_thread_;
    size_t max_thread_;
	size_t bulk_size_;
    std::string name_;
    Vector<std::jthread> threads_;
    Vector<std::atomic<ExecuteFlags>> task_execute_flags_;
    SharedTaskQueuePtr task_pool_;
    Vector<WorkStealingTaskQueuePtr> task_work_queues_;    
    std::latch work_done_;
    std::latch start_clean_up_;
    LoggerPtr logger_;
};

class ThreadPoolExecutor final : public IThreadPoolExecutor {
public:
	explicit ThreadPoolExecutor(const std::string_view& name,
	                            uint32_t max_thread = std::thread::hardware_concurrency(),
								size_t bulk_size = std::thread::hardware_concurrency() / 2,
	                            ThreadPriority priority = ThreadPriority::PRIORITY_NORMAL);

	~ThreadPoolExecutor() override;
    
	XAMP_DISABLE_COPY(ThreadPoolExecutor)

    void Stop() override;

    size_t GetThreadSize() const override;
};

XAMP_BASE_NAMESPACE_END
