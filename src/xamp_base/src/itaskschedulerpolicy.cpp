#include <base/platform.h>
#include <base/itaskschedulerpolicy.h>

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN
#define XAMP_NO_TLS_GUARDS [[msvc::no_tls_guard]]
#else
#define XAMP_NO_TLS_GUARDS
#endif

AlignPtr<ITaskSchedulerPolicy> MakeTaskSchedulerPolicy(TaskSchedulerPolicy policy) {
	switch (policy) {
	case TaskSchedulerPolicy::ROUND_ROBIN_POLICY:
		return MakeAlign<ITaskSchedulerPolicy, RoundRobinSchedulerPolicy>();
	case TaskSchedulerPolicy::THREAD_LOCAL_RANDOM_POLICY:
	default:
		return MakeAlign<ITaskSchedulerPolicy, ThreadLocalRandomSchedulerPolicy>();
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

	for (size_t n = 0; n < max_thread * K; ++n) {
		auto current = n % max_thread;
		if (thread_execute_flags[current] == ExecuteFlags::EXECUTE_LONG_RUNNING) {
			continue;
		}
		auto& queue = task_work_queues.at(current);
		if (queue->TryEnqueue(std::forward<MoveOnlyFunction>(task))) {
			thread_execute_flags[current] = flags;
			return;
		}
	}
	task_pool->Enqueue(task);
}

size_t RoundRobinSchedulerPolicy::ScheduleNext([[maybe_unused]] size_t index,
	[[maybe_unused]] const Vector<WorkStealingTaskQueuePtr>& work_queues,
	const Vector<std::atomic<ExecuteFlags>>& thread_execute_flags) {
	static std::atomic<int32_t> id{ -1 };
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

void ThreadLocalRandomSchedulerPolicy::SetMaxThread(size_t max_thread) {
	max_thread_ = max_thread;
}

size_t ThreadLocalRandomSchedulerPolicy::ScheduleNext(size_t index,
	[[maybe_unused]] const Vector<WorkStealingTaskQueuePtr>& work_queues,
	const Vector<std::atomic<ExecuteFlags>>& thread_execute_flags) {
	XAMP_NO_TLS_GUARDS static thread_local auto prng = Sfc64Engine<>();		
	uint32_t random_index = 0;
	while (true) {
		random_index = prng() % static_cast<uint32_t>(max_thread_);
		if (thread_execute_flags[random_index] != ExecuteFlags::EXECUTE_LONG_RUNNING) {
			// Avoid self stealing
			if (random_index == index) {
				return (std::numeric_limits<size_t>::max)();
			}
			return random_index;
		}		
	}
}

XAMP_BASE_NAMESPACE_END
