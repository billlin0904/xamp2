#include <algorithm>
#include <sstream>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/platform.h>
#include <base/threadpool.h>

namespace xamp::base {

inline constexpr auto kDefaultTimeout = std::chrono::milliseconds(100);
inline constexpr auto kSharedTaskQueueSize = 4096;
inline constexpr auto kWorkStealingTaskQueueSize = 4096;
inline constexpr auto kMaxStealFailureSize = 500;
inline constexpr auto kMaxWorkQueueSize = 65536;

TaskScheduler::TaskScheduler(const std::string_view& pool_name, uint32_t max_thread, CpuAffinity affinity, ThreadPriority priority)
	: TaskScheduler(TaskSchedulerPolicy::RANDOM_POLICY, TaskStealPolicy::CONTINUATION_STEALING_POLICY, pool_name, max_thread, affinity, priority)  {
}

TaskScheduler::TaskScheduler(TaskSchedulerPolicy policy, TaskStealPolicy steal_policy, const std::string_view& pool_name, uint32_t max_thread, CpuAffinity affinity, ThreadPriority priority)
	: is_stopped_(false)
	, running_thread_(0)
	, max_thread_(max_thread)
	, pool_name_(pool_name)
	, task_steal_policy_(MakeTaskStealPolicy(steal_policy))
	, task_scheduler_policy_(MakeTaskSchedulerPolicy(policy)) {
	logger_ = LoggerManager::GetInstance().GetLogger(pool_name.data());
	try {
		task_pool_ = MakeAlign<SharedTaskQueue>(kSharedTaskQueueSize);
		task_scheduler_policy_->SetMaxThread(max_thread_);

		for (size_t i = 0; i < max_thread_; ++i) {
			task_work_queues_.push_back(MakeAlign<WorkStealingTaskQueue>(kMaxWorkQueueSize));
		}
		for (size_t i = 0; i < max_thread_; ++i) {
            AddThread(i, affinity, priority);
		}
	}
	catch (...) {
		is_stopped_ = true;
		throw;
	}	
	XAMP_LOG_D(logger_, "TaskScheduler initial max thread:{} affinity:{} priority:{}",
		max_thread, affinity, priority);
}

TaskScheduler::~TaskScheduler() {
	Destroy();
}

uint32_t TaskScheduler::GetThreadSize() const {
	return max_thread_;
}

void TaskScheduler::SubmitJob(Task&& task) {
	auto* policy = task_scheduler_policy_.get();
	task_steal_policy_->SubmitJob(std::move(task),
		max_thread_,
		task_pool_.get(),
		policy, 
		task_work_queues_);
}

void TaskScheduler::Destroy() noexcept {
	if (!task_pool_ || threads_.empty()) {
		return;
	}

	XAMP_LOG_D(logger_, "Thread pool start destory.");

	is_stopped_ = true;
	task_pool_->WakeupForShutdown();

	for (size_t i = 0; i < max_thread_; ++i) {
		try {
			if (threads_.at(i).joinable()) {
				threads_.at(i).join();
			}
		}
		catch (...) {
		}
		XAMP_LOG_D(logger_, "Worker Thread {} joined.", i);
	}

	task_pool_.reset();
	threads_.clear();
	task_work_queues_.clear();
	task_scheduler_policy_.reset();
    task_steal_policy_.reset();

	XAMP_LOG_D(logger_, "Thread pool was destory.");
}

std::optional<Task> TaskScheduler::TryDequeueSharedQueue(std::chrono::milliseconds timeout) {
	Task task;
	if (task_pool_->Dequeue(task, timeout)) {
		XAMP_LOG_D(logger_, "Pop shared queue.");
		return std::move(task);
	}
	return std::nullopt;
}

std::optional<Task> TaskScheduler::TryDequeueSharedQueue() {
	Task task;
    if (task_pool_->TryDequeue(task)) {
		XAMP_LOG_D(logger_, "Pop shared queue.");
		return std::move(task);
	}
	return std::nullopt;
}

std::optional<Task> TaskScheduler::TryLocalPop(WorkStealingTaskQueue* local_queue) const {
	Task task;
	if (local_queue->TryDequeue(task)) {
		XAMP_LOG_D(logger_, "Pop local queue ({}).", local_queue->size());
		return std::move(task);
	}
	return std::nullopt;
}

std::optional<Task> TaskScheduler::TrySteal(size_t i) {
	Task task;
	for (size_t n = 0; n != max_thread_; ++n) {
		if (is_stopped_) {
			return std::nullopt;
		}

		const auto index = (i + n) % max_thread_;
		if (task_work_queues_.at(index)->TryDequeue(task)) {
			XAMP_LOG_D(logger_, "Steal other thread {} queue.", index);
			return std::move(task);
		}
	}
	return std::nullopt;
}

void TaskScheduler::SetWorkerThreadName(size_t i) {
#ifdef XAMP_OS_MAC
	// Sleep for set thread name.
	std::this_thread::sleep_for(std::chrono::milliseconds(900));
#endif
	std::ostringstream stream;
	stream << "Worker Thread(" << i << ") " << pool_name_;
	SetThreadName(stream.str());
}

void TaskScheduler::AddThread(size_t i, CpuAffinity affinity, ThreadPriority priority) {
	threads_.emplace_back([i, this, priority]() mutable {
		// Avoid 64K Aliasing in L1 Cache (Intel hyper-threading)
		const auto L1_padding_buffer =
			MakeStackBuffer((std::min)(kInitL1CacheLineSize * i,
			kMaxL1CacheLineSize));

		SetWorkerThreadName(i);
		SetThreadPriority(priority);

		auto* local_queue = task_work_queues_[i].get();
		auto* policy = task_scheduler_policy_.get();
		auto steal_failure_count = 0;
		auto thread_id = GetCurrentThreadId();

		XAMP_LOG_D(logger_, "Worker Thread {} ({}) start.", thread_id, i);

		while (!is_stopped_) {
			auto task = TryLocalPop(local_queue);

			if (!task) {
				const auto steal_index = policy->ScheduleNext(i, task_work_queues_);

				task = TrySteal(steal_index);

				if (!task) {
					++steal_failure_count;
					if (steal_failure_count >= kMaxStealFailureSize) {
						task = TryDequeueSharedQueue(kDefaultTimeout);
					}
				}
			}

			if (!task) {
				CpuRelax();
				continue;
			} else {
				steal_failure_count = 0;
			}

			auto running_thread = ++running_thread_;
			XAMP_LOG_D(logger_, "Worker Thread {} ({}) weakup, running:{}", i, thread_id, running_thread);
			(*task)(i);
			--running_thread_;
			XAMP_LOG_D(logger_, "Worker Thread {} ({}) execute finished.", i, thread_id);
		}

		XAMP_LOG_D(logger_, "Worker Thread {} is existed.", i);
		});

	if (affinity != kDefaultAffinityCpuCore) {
		SetThreadAffinity(threads_.at(i), affinity);
		XAMP_LOG_D(logger_, "Worker Thread {} affinity:{}.", i, affinity);
	}
}

ThreadPool::ThreadPool(const std::string_view& pool_name, TaskSchedulerPolicy policy, TaskStealPolicy steal_policy, uint32_t max_thread, CpuAffinity affinity, ThreadPriority priority)
	: IThreadPool(MakeAlign<ITaskScheduler, TaskScheduler>(policy, steal_policy, pool_name, (std::min)(max_thread, kMaxThread), affinity, priority)) {
}

ThreadPool::ThreadPool(const std::string_view& pool_name, uint32_t max_thread, CpuAffinity affinity, ThreadPriority priority)
	: IThreadPool(MakeAlign<ITaskScheduler, TaskScheduler>(pool_name, (std::min)(max_thread, kMaxThread), affinity, priority)) {
}

ThreadPool::~ThreadPool() {
	Stop();
}

uint32_t ThreadPool::GetThreadSize() const {
	return scheduler_->GetThreadSize();
}

void ThreadPool::Stop() {
	dynamic_cast<TaskScheduler*>(scheduler_.get())->Destroy();
}

}
