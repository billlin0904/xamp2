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
#include <base/stopwatch.h>
#include <base/align_ptr.h>
#include <base/bounded_queue.h>
#include <base/ithreadpool.h>

#include <base/logger.h>

namespace xamp::base {
	
class TaskScheduler final : public ITaskScheduler {
public:
    TaskScheduler(const std::string_view & pool_name, size_t max_thread, int32_t affinity);
	
    XAMP_DISABLE_COPY(TaskScheduler)

    virtual ~TaskScheduler() noexcept;

    void SubmitJob(Task&& task) override;

    void SetAffinityMask(int32_t core) override;

    void Destroy() noexcept override;

private:
    static void SetWorkerThreadName(size_t i);
	
    std::optional<Task> TryPopPoolQueue();

    std::optional<Task> TryPopLocalQueue(size_t index);

    std::optional<Task> TrySteal(size_t i);

    void AddThread(size_t i);

    using TaskQueue = BoundedQueue<Task, FastMutex, FastConditionVariable>;
    using SharedTaskQueuePtr = AlignPtr<TaskQueue>;
    
    static constexpr size_t K = 4;
    static constexpr size_t kInitL1CacheLineSize = 64 * 1024;
    static constexpr size_t kMaxL1CacheLineSize = 256 * 1024;

	std::atomic<bool> is_stopped_;
    std::atomic<size_t> running_thread_;
    int32_t affinity_;
	size_t index_;
    size_t max_thread_;
    TaskQueue pool_queue_;
    std::vector<std::thread> threads_;
    std::vector<SharedTaskQueuePtr> shared_queues_;
    std::shared_ptr<spdlog::logger> logger_;
};

class ThreadPool final : public IThreadPool {
public:
    ThreadPool(const std::string_view& pool_name, uint32_t max_thread = std::thread::hardware_concurrency(), int32_t affinity = kDefaultAffinityCpuCore);

    virtual ~ThreadPool();
    
	XAMP_DISABLE_COPY(ThreadPool)

    template <typename F, typename... Args>
    decltype(auto) Spawn(F&& f, Args&&... args);

    void Stop();

    void SetAffinityMask(int32_t core);
};

}
