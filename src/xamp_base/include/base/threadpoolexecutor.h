//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
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

namespace xamp::base {

class TaskScheduler final : public ITaskScheduler {
public:
    TaskScheduler(const std::string_view & pool_name,
        uint32_t max_thread, 
        CpuAffinity affinity,
        ThreadPriority priority);

    TaskScheduler(TaskSchedulerPolicy policy,
        TaskStealPolicy steal_policy,
        const std::string_view& pool_name,
        uint32_t max_thread, 
        CpuAffinity affinity,
        ThreadPriority priority);
	
    XAMP_DISABLE_COPY(TaskScheduler)

    ~TaskScheduler() override;

    uint32_t GetThreadSize() const override;

    void SubmitJob(MoveableFunction&& task) override;

    void Destroy() noexcept override;
private:
    void SetWorkerThreadName(size_t i);

    std::optional<MoveableFunction> TryDequeueSharedQueue();

    std::optional<MoveableFunction> TrySteal(StopToken const & stop_token, size_t i);

    std::optional<MoveableFunction> TryLocalPop(WorkStealingTaskQueue * local_queue) const;

    std::optional<MoveableFunction> TryDequeueSharedQueue(std::chrono::milliseconds timeout);

    void AddThread(size_t i, CpuAffinity affinity, ThreadPriority priority);

    std::atomic<bool> is_stopped_;
    std::atomic<size_t> running_thread_;
    uint32_t max_thread_;
    std::string pool_name_;
    Vector<JThread> threads_;
    SharedTaskQueuePtr task_pool_;
    AlignPtr<ITaskStealPolicy> task_steal_policy_;
    AlignPtr<ITaskSchedulerPolicy> task_scheduler_policy_;
    Vector<WorkStealingTaskQueuePtr> task_work_queues_;
    Latch work_done_;
    Latch start_clean_up_;
    std::shared_ptr<Logger> logger_;
};

class ThreadPoolExecutor final : public IThreadPoolExecutor {
public:
	ThreadPoolExecutor(const std::string_view& pool_name,
        TaskSchedulerPolicy policy,
        TaskStealPolicy steal_policy,
	    uint32_t max_thread = std::thread::hardware_concurrency(), 
        CpuAffinity affinity = kDefaultAffinityCpuCore,
	    ThreadPriority priority = ThreadPriority::NORMAL);

    explicit ThreadPoolExecutor(const std::string_view& pool_name,
        uint32_t max_thread = std::thread::hardware_concurrency(),
        CpuAffinity affinity = kDefaultAffinityCpuCore,
        ThreadPriority priority = ThreadPriority::NORMAL);

	~ThreadPoolExecutor() override;
    
	XAMP_DISABLE_COPY(ThreadPoolExecutor)

    void Stop() override;

    uint32_t GetThreadSize() const override;
};

}
