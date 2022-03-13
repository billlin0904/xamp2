//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <future>
#include <atomic>
#include <vector>
#include <optional>
#include <type_traits>
#include <memory>

#include <base/rng.h>
#include <base/base.h>
#include <base/align_ptr.h>
#include <base/blocking_queue.h>
#include <base/ithreadpool.h>
#include <base/platform.h>
#include <base/lifoqueue.h>
#include <base/logger.h>

namespace xamp::base {

using SharedTaskQueue = BlockingQueue<Task, FastMutex, FastConditionVariable>;
using SharedTaskQueuePtr = AlignPtr<SharedTaskQueue>;

//using WorkStealingTaskQueue = BlockingQueue<Task, FastMutex, FastConditionVariable, LIFOQueue<Task>>;
using WorkStealingTaskQueue = BlockingQueue<Task, FastMutex, FastConditionVariable>;
//using WorkStealingTaskQueue = LockFreeStack<Task>;
using WorkStealingTaskQueuePtr = AlignPtr<WorkStealingTaskQueue>;

class TaskScheduler final : public ITaskScheduler {
public:
    TaskScheduler(const std::string_view & pool_name, size_t max_thread, int32_t affinity, ThreadPriority priority);
	
    XAMP_DISABLE_COPY(TaskScheduler)

    ~TaskScheduler() override;

    void SubmitJob(Task&& task) override;

    void Destroy() noexcept override;

private:
    static void SetWorkerThreadName(size_t i);

    std::optional<Task> TryDequeueSharedQueue();

    std::optional<Task> TrySteal(size_t i);

    std::optional<Task> TryLocalPop(WorkStealingTaskQueue * local_queue) const;

    std::optional<Task> TryDequeueSharedQueue(std::chrono::milliseconds timeout);

    void AddThread(size_t i, int32_t affinity, ThreadPriority priority);

    static constexpr size_t K = 4;
    static constexpr size_t kInitL1CacheLineSize = 64 * 1024;
    static constexpr size_t kMaxL1CacheLineSize = 256 * 1024;

	std::atomic<bool> is_stopped_;
    std::atomic<size_t> running_thread_;
	size_t index_;
    size_t max_thread_;
    std::vector<std::thread> threads_;
    SharedTaskQueuePtr shared_queues_;
    std::vector<WorkStealingTaskQueuePtr> workstealing_queue_list_;
    std::shared_ptr<spdlog::logger> logger_;
};

class ThreadPool final : public IThreadPool {
public:
	explicit ThreadPool(const std::string_view& pool_name, 
	                    uint32_t max_thread = std::thread::hardware_concurrency(), 
	                    int32_t affinity = kDefaultAffinityCpuCore,
	                    ThreadPriority priority = ThreadPriority::NORMAL);

	~ThreadPool() override;
    
	XAMP_DISABLE_COPY(ThreadPool)

    void Stop() override;
};

}
