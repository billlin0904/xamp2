//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <base/base.h>
#include <base/align_ptr.h>
#include <base/ithreadpool.h>
#include <base/blocking_queue.h>

namespace xamp::base {

using SharedTaskQueue = BlockingQueue<Task>;
using SharedTaskQueuePtr = AlignPtr<SharedTaskQueue>;

using WorkStealingTaskQueue = BlockingQueue<Task>;

using SharedTaskQueuePtr = AlignPtr<SharedTaskQueue>;
using WorkStealingTaskQueuePtr = AlignPtr<WorkStealingTaskQueue>;

class XAMP_BASE_API ITaskSchedulerPolicy {
public:
    XAMP_BASE_CLASS(ITaskSchedulerPolicy)

	virtual void SetMaxThread(size_t max_thread) = 0;

    virtual size_t ScheduleNext(size_t cur_index, const std::vector<WorkStealingTaskQueuePtr> &work_queues) = 0;
protected:
    ITaskSchedulerPolicy() = default;
};

XAMP_BASE_API AlignPtr<ITaskSchedulerPolicy> MakeTaskSchedulerPolicy(TaskSchedulerPolicy policy);

class RoundRobinSchedulerPolicy final : public ITaskSchedulerPolicy {
public:
    void SetMaxThread(size_t max_thread) override;

    size_t ScheduleNext(size_t cur_index, const std::vector<WorkStealingTaskQueuePtr>& work_queues) override;
private:
    size_t max_thread_{ 0 };
};

class RandomSchedulerPolicy final : public ITaskSchedulerPolicy {
public:
    void SetMaxThread(size_t max_thread) override;

    size_t ScheduleNext(size_t cur_index, const std::vector<WorkStealingTaskQueuePtr>& work_queues) override;
private:
    AlignVector<PRNG> prngs_;
};

class LeastLoadSchedulerPolicy final : public ITaskSchedulerPolicy {
public:
    void SetMaxThread(size_t max_thread) override;

    size_t ScheduleNext(size_t cur_index, const std::vector<WorkStealingTaskQueuePtr>& work_queues) override;
};

}

