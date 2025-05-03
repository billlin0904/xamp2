//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
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

/********************************************************************
 * ExponentialMovingAverage: 指數移動平均 (EMA)
 ********************************************************************/
 class ExponentialMovingAverage {
 public:
     // 建構子: 指定平滑係數 alpha (default=0.2)
     explicit ExponentialMovingAverage(double alpha = 0.2)
         : has_value_(false)
         , alpha_(alpha)
         , ema_value_(0.0) {
     }

     // 更新 EMA
     void Update(double new_value) noexcept {
         if (!has_value_) {
             // 第一次直接設定
             ema_value_ = new_value;
             has_value_ = true;
         }
         else {
             // EMA公式：ema_new = alpha * x_new + (1 - alpha) * ema_old
             ema_value_ = alpha_ * new_value + (1.0 - alpha_) * ema_value_;
         }
     }

     // 取得目前的 EMA 值
     double GetValue() const noexcept {
         return ema_value_;
     }

 private:
     bool has_value_;    // 是否已初始化過
     double alpha_;      // 平滑係數(0<alpha<=1)
     double ema_value_;  // 當前的 EMA 結果
};


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
	              size_t bulk_size,
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
    size_t TryDequeueSharedQueue(std::vector<MoveOnlyFunction>& tasks,
        const std::stop_token& stop_token);

    /*
    * Try dequeue task from shared queue.
    */
    size_t TryDequeueSharedQueue(std::vector<MoveOnlyFunction>& tasks,
        const std::stop_token& stop_token, 
        std::chrono::milliseconds timeout);

    /*
    * Try steal task from other thread.
    */
    size_t TrySteal(std::vector<MoveOnlyFunction> &tasks,
        const std::stop_token& stop_token,
        size_t random_start,
        size_t current_thread_index);

    /*
    * Try dequeue task from local queue.
    */
    size_t TryLocalPop(std::vector<MoveOnlyFunction>& tasks,
        const std::stop_token& stop_token,
        WorkStealingTaskQueue* local_queue) const;

    /*
    * Add thread to thread pool.
    */
    void AddThread(size_t i, ThreadPriority priority);

    /*
	 * Execute task.
	 */
    void Execute(std::vector<MoveOnlyFunction>& tasks,
        size_t task_size,
        size_t current_index,
        const std::stop_token& stop_token);

	bool IsLongRunning(size_t index) const;

    std::atomic<bool> is_stopped_;
    std::atomic<size_t> running_thread_;
    size_t max_thread_;
	size_t bulk_size_;
    std::string name_;
    std::vector<std::jthread> threads_;
    std::vector<std::atomic<ExecuteFlags>> task_execute_flags_;
    std::vector<std::atomic<size_t>> attempt_count_;
    std::vector<std::atomic<size_t>> success_count_;
    SharedTaskQueuePtr task_pool_;
    std::vector<WorkStealingTaskQueuePtr> task_work_queues_;
    std::vector<ExponentialMovingAverage> ema_list_;
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
