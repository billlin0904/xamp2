//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

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

    virtual size_t ScheduleNext(size_t cur_index, size_t max_thread, const std::vector<WorkStealingTaskQueuePtr> &work_queues) const = 0;
protected:
    ITaskSchedulerPolicy() = default;
};

XAMP_BASE_API AlignPtr<ITaskSchedulerPolicy> MakeTaskSchedulerPolicy(TaskSchedulerPolicy policy);

class RoundRobinSchedulerPolicy : public ITaskSchedulerPolicy {
public:
    size_t ScheduleNext(size_t cur_index, size_t max_thread, const std::vector<WorkStealingTaskQueuePtr>& work_queues) const override;
};

class RandomSchedulerPolicy : public ITaskSchedulerPolicy {
public:
    size_t ScheduleNext(size_t cur_index, size_t max_thread, const std::vector<WorkStealingTaskQueuePtr>& work_queues) const override;
};

class LeastLoadSchedulerPolicy : public ITaskSchedulerPolicy {
public:
    size_t ScheduleNext(size_t cur_index, size_t max_thread, const std::vector<WorkStealingTaskQueuePtr>& work_queues) const override;
};

}

