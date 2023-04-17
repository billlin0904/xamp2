//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/align_ptr.h>
#include <base/rng.h>
#include <base/ithreadpoolexecutor.h>
#include <base/workstealingtaskqueue.h>

XAMP_BASE_NAMESPACE_BEGIN

class XAMP_BASE_API ITaskSchedulerPolicy {
public:
    XAMP_BASE_CLASS(ITaskSchedulerPolicy)

	virtual void SetMaxThread(size_t max_thread) = 0;

    virtual size_t ScheduleNext(size_t index, const Vector<WorkStealingTaskQueuePtr>& work_queues) = 0;
protected:
    ITaskSchedulerPolicy() = default;
};

class XAMP_BASE_API ITaskStealPolicy {
public:
    XAMP_BASE_CLASS(ITaskStealPolicy)

    virtual void SubmitJob(MoveOnlyFunction&& task,
        size_t max_thread,
        SharedTaskQueue* task_pool,
        ITaskSchedulerPolicy* policy,
        const Vector<WorkStealingTaskQueuePtr>& task_work_queues) = 0;
protected:
    ITaskStealPolicy() = default;
};

class ContinuationStealPolicy final : public ITaskStealPolicy{
public:
    void SubmitJob(MoveOnlyFunction&& task,
        size_t max_thread,
        SharedTaskQueue* task_pool,
        ITaskSchedulerPolicy* policy,
        const Vector<WorkStealingTaskQueuePtr>& task_work_queues) override;
};

XAMP_BASE_API AlignPtr<ITaskSchedulerPolicy> MakeTaskSchedulerPolicy(TaskSchedulerPolicy policy);

XAMP_BASE_API AlignPtr<ITaskStealPolicy> MakeTaskStealPolicy(TaskStealPolicy policy);

class RoundRobinSchedulerPolicy final : public ITaskSchedulerPolicy {
public:
    void SetMaxThread(size_t max_thread) override;

    size_t ScheduleNext(size_t index, const Vector<WorkStealingTaskQueuePtr>& work_queues) override;
private:
    size_t max_thread_{ 0 };
};

class ThreadLocalRandomSchedulerPolicy final : public ITaskSchedulerPolicy {
public:
    void SetMaxThread(size_t max_thread) override;

    size_t ScheduleNext(size_t index, const Vector<WorkStealingTaskQueuePtr>& work_queues) override;
private:
    size_t max_thread_{ 0 };
};

class RandomSchedulerPolicy final : public ITaskSchedulerPolicy {
public:
    void SetMaxThread(size_t max_thread) override;

    size_t ScheduleNext(size_t index, const Vector<WorkStealingTaskQueuePtr>& work_queues) override;
private:
    Vector<Sfc64Engine<>> prngs_;
};

class LeastLoadSchedulerPolicy final : public ITaskSchedulerPolicy {
public:
    void SetMaxThread(size_t max_thread) override;

    size_t ScheduleNext(size_t index, const Vector<WorkStealingTaskQueuePtr>& work_queues) override;
};

XAMP_BASE_NAMESPACE_END

