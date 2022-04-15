#include <base/rng.h>
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

size_t RoundRobinSchedulerPolicy::ScheduleNext(size_t /*cur_index*/, size_t max_thread, const std::vector<WorkStealingTaskQueuePtr>& work_queues) const {
	static std::atomic<int32_t> index{ -1 };
	return (index.fetch_add(1, std::memory_order_relaxed) + 1) % max_thread;
}

size_t RandomSchedulerPolicy::ScheduleNext(size_t cur_index, size_t max_thread, const std::vector<WorkStealingTaskQueuePtr>& work_queues) const {
	size_t random_index;
	do {
		random_index = PRNG::GetInstance().NextSize(max_thread - 1);
	} while (random_index == cur_index);

	return random_index;
}

size_t LeastLoadSchedulerPolicy::ScheduleNext(size_t /*cur_index*/, size_t max_thread, const std::vector<WorkStealingTaskQueuePtr>& work_queues) const {
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
