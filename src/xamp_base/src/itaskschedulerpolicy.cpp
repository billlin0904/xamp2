#include <base/platform.h>
#include <base/itaskschedulerpolicy.h>

XAMP_BASE_NAMESPACE_BEGIN

AlignPtr<ITaskSchedulerPolicy> MakeTaskSchedulerPolicy(TaskSchedulerPolicy policy) {
	switch (policy) {
	case TaskSchedulerPolicy::ROUND_ROBIN_POLICY:
		return MakeAlign<ITaskSchedulerPolicy, RoundRobinSchedulerPolicy>();
	case TaskSchedulerPolicy::THREAD_LOCAL_RANDOM_POLICY:
	default:
		return MakeAlign<ITaskSchedulerPolicy, RandomSchedulerPolicy>();
	}
}

AlignPtr<ITaskStealPolicy> MakeTaskStealPolicy(TaskStealPolicy policy) {
	return MakeAlign<ITaskStealPolicy, ContinuationStealPolicy>();
}

void ContinuationStealPolicy::SubmitJob(MoveOnlyFunction&& task,
                                        ExecuteFlags flags,
                                        size_t max_thread,
                                        SharedTaskQueue* task_pool,
                                        ITaskSchedulerPolicy* policy,
                                        const Vector<WorkStealingTaskQueuePtr>& task_work_queues,
                                        Vector<std::atomic<ExecuteFlags>>& thread_execute_flags) {
	static constexpr size_t K = 4;
	XAMP_NO_TLS_GUARDS static thread_local PRNG prng;

	for (size_t n = 0; n < max_thread * K; ++n) {
		const auto current = prng(size_t(0), max_thread - 1);
		if (thread_execute_flags[current] != ExecuteFlags::EXECUTE_LONG_RUNNING) {
			auto& queue = task_work_queues.at(current);
			if (queue->TryEnqueue(std::move(task))) {
				thread_execute_flags[current] = flags;
				return;
			}
		}
	}
	task_pool->Enqueue(task);
}

size_t RoundRobinSchedulerPolicy::ScheduleNext([[maybe_unused]] size_t index,
                                               [[maybe_unused]] const Vector<WorkStealingTaskQueuePtr>& work_queues,
                                               const Vector<std::atomic<ExecuteFlags>>& thread_execute_flags) {
	static std::atomic<int32_t> id{-1};
	while (true) {
		auto index = (id.fetch_add(1, std::memory_order_relaxed) + 1) % max_thread_;
		if (thread_execute_flags[index] != ExecuteFlags::EXECUTE_LONG_RUNNING) {
			return index;
		}
	}
}

void RoundRobinSchedulerPolicy::SetMaxThread(size_t max_thread) {
	max_thread_ = max_thread;
}

void RandomSchedulerPolicy::SetMaxThread(size_t max_thread) {
	max_thread_ = max_thread;
}

size_t RandomSchedulerPolicy::ScheduleNext(size_t index,
                                           [[maybe_unused]] const Vector<WorkStealingTaskQueuePtr>& work_queues,
                                           const Vector<std::atomic<ExecuteFlags>>& thread_execute_flags) {
	// use Sfc64 random engine
	XAMP_NO_TLS_GUARDS static thread_local PRNG prng;
	constexpr size_t kMaxAttempts = 100;
	uint32_t random_index = 0;

	for (size_t attempt = 0; attempt < kMaxAttempts; ++attempt) {
		random_index = prng(size_t(0), max_thread_ - 1);
		if (random_index != index
			&& thread_execute_flags[random_index] != ExecuteFlags::EXECUTE_LONG_RUNNING) {
			return random_index;
		}
	}
	return kInvalidScheduleIndex;
}

XAMP_BASE_NAMESPACE_END
