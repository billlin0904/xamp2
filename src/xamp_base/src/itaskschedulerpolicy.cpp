#include <base/rng.h>
#include <base/platform.h>
#include <base/itaskschedulerpolicy.h>

namespace xamp::base {

AlignPtr<ITaskSchedulerPolicy> MakeTaskSchedulerPolicy(TaskSchedulerPolicy policy) {
	switch (policy) {
	case TaskSchedulerPolicy::LEAST_LOAD_POLICY:
		return MakeAlign<ITaskSchedulerPolicy, LeastLoadSchedulerPolicy>();
	case TaskSchedulerPolicy::ROUND_ROBIN_POLICY:
		return MakeAlign<ITaskSchedulerPolicy, RoundRobinSchedulerPolicy>();
	case TaskSchedulerPolicy::RANDOM_POLICY:
	default:
		return MakeAlign<ITaskSchedulerPolicy, RandomSchedulerPolicy>();
	}
}

AlignPtr<ITaskStealPolicy> MakeTaskStealPolicy(TaskStealPolicy policy) {
	return MakeAlign<ITaskStealPolicy, ContinuationStealPolicy>();
}

void ContinuationStealPolicy::SubmitJob(MoveableFunction&& task,
	size_t max_thread,
	SharedTaskQueue* task_pool,
	ITaskSchedulerPolicy* policy,
	const Vector<WorkStealingTaskQueuePtr>& task_work_queues) {
	static constexpr size_t K = 4;

	for (size_t n = 0; n < max_thread * K; ++n) {
		auto current = n % max_thread;
		auto& queue = task_work_queues.at(current);
		if (queue->TryEnqueue(task)) {
			return;
		}
	}
	task_pool->Enqueue(task);
}

size_t RoundRobinSchedulerPolicy::ScheduleNext([[maybe_unused]] size_t index,
	[[maybe_unused]] const Vector<WorkStealingTaskQueuePtr>& work_queues) {
	static std::atomic<int32_t> id{ -1 };
	return (id.fetch_add(1, std::memory_order_relaxed) + 1) % max_thread_;
}

void RoundRobinSchedulerPolicy::SetMaxThread(size_t max_thread) {
	max_thread_ = max_thread;
}

void RandomSchedulerPolicy::SetMaxThread(size_t max_thread) {
	prngs_.resize(max_thread);

	for (size_t i = 0; i < max_thread; ++i) {
		prngs_[i].seed(GenRandomSeed());
		for (size_t jump = 0; jump < i; ++jump) {
			prngs_[i].Jump();
		}
	}
}

size_t RandomSchedulerPolicy::ScheduleNext(size_t index,
	[[maybe_unused]] const Vector<WorkStealingTaskQueuePtr>& work_queues) {
	auto& prng = prngs_[index];
	const auto random_index = prng() % prngs_.size();
	if (random_index == index) {
		return (std::numeric_limits<size_t>::max)();
	}
	return random_index;
}

void LeastLoadSchedulerPolicy::SetMaxThread([[maybe_unused]] size_t max_thread) {
}

size_t LeastLoadSchedulerPolicy::ScheduleNext([[maybe_unused]] size_t index, const Vector<WorkStealingTaskQueuePtr>& work_queues) {
	size_t min_wq_size_index = 0;
	size_t min_wq_size = 0;
	size_t i = 0;
	for (const auto &wq : work_queues) {
		const auto queue_size = wq->size();
		if (queue_size < min_wq_size) {
			min_wq_size_index = i;
			min_wq_size = queue_size;
		}
		++i;
	}
	return min_wq_size_index;
}

}
