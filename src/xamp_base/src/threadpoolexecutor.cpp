#include <base/threadpoolexecutor.h>

#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/platform.h>
#include <base/stopwatch.h>
#include <base/latch.h>

#include <algorithm>
#include <sstream>

namespace xamp::base {

inline constexpr auto kDefaultTimeout = std::chrono::milliseconds(100);
inline constexpr auto kSharedTaskQueueSize = 4096;
inline constexpr auto kWorkStealingTaskQueueSize = 4096;
inline constexpr auto kMaxStealFailureSize = 500;
inline constexpr auto kMaxWorkQueueSize = 65536;

TaskScheduler::TaskScheduler(const std::string_view& pool_name, size_t max_thread, CpuAffinity affinity, ThreadPriority priority)
	: TaskScheduler(TaskSchedulerPolicy::THREAD_LOCAL_RANDOM_POLICY, TaskStealPolicy::CONTINUATION_STEALING_POLICY, pool_name, max_thread, affinity, priority)  {
}

TaskScheduler::TaskScheduler(TaskSchedulerPolicy policy, TaskStealPolicy steal_policy, const std::string_view& pool_name, size_t max_thread, CpuAffinity affinity, ThreadPriority priority)
	: is_stopped_(false)
	, running_thread_(0)
	, last_idle_thread_count_(max_thread - 1)
	, max_thread_(max_thread)
	, min_thread_(1)
	, thread_priority_(priority)
	, pool_name_(pool_name)
	, task_steal_policy_(MakeTaskStealPolicy(steal_policy))
	, task_scheduler_policy_(MakeTaskSchedulerPolicy(policy))
	, work_done_(max_thread_)
	, start_clean_up_(1) {
	logger_ = LoggerManager::GetInstance().GetLogger(pool_name);
	try {
		task_pool_ = MakeAlign<SharedTaskQueue>(kSharedTaskQueueSize);
		task_scheduler_policy_->SetMaxThread(max_thread_);

		for (size_t i = 0; i < max_thread_; ++i) {
			task_work_queues_.push_back(MakeAlign<WorkStealingTaskQueue>(kMaxWorkQueueSize));
		}
		for (size_t i = 0; i < max_thread_; ++i) {
            AddThread(i, priority);
		}
	}
	catch (...) {
		is_stopped_ = true;
		throw;
	}

	work_done_.wait();

	// 因為macOS 不支援thread running狀態設置affinity,
	// 所以都改由此方式初始化affinity.
	JThread([this, priority, affinity] {
		for (size_t i = 0; i < max_thread_; ++i) {
#ifndef XAMP_OS_WIN
			SetThreadPriority(threads_.at(i), priority);
#endif
			if (affinity != kDefaultAffinityCpuCore) {
				SetThreadAffinity(threads_.at(i), affinity);
				XAMP_LOG_D(logger_, "Worker Thread {} affinity:{}.", i, affinity);
			}
		}
		XAMP_LOG_D(logger_, "Set Thread affinity, priority is success.");
		start_clean_up_.count_down();
		}).detach();

	//CreateIdleThread();

	XAMP_LOG_D(logger_,
		"TaskScheduler initial max thread:{} affinity:{} priority:{}",
		max_thread, affinity, priority);
}

TaskScheduler::~TaskScheduler() {
	Destroy();
}

size_t TaskScheduler::GetThreadSize() const {
	return max_thread_;
}

void TaskScheduler::SubmitJob(MoveOnlyFunction&& task) {
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
		threads_.at(i).request_stop();

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

std::optional<MoveOnlyFunction> TaskScheduler::TryDequeueSharedQueue(std::chrono::milliseconds timeout) {
	MoveOnlyFunction func;
	if (task_pool_->Dequeue(func, timeout)) {
		XAMP_LOG_D(logger_, "Pop shared queue.");
		return std::move(func);
	}
	return std::nullopt;
}

std::optional<MoveOnlyFunction> TaskScheduler::TryDequeueSharedQueue() {
    if (auto func = task_pool_->TryDequeue()) {
		XAMP_LOG_D(logger_, "Pop shared queue.");
	}
	return std::nullopt;
}

std::optional<MoveOnlyFunction> TaskScheduler::TryLocalPop(WorkStealingTaskQueue* local_queue) const {
	MoveOnlyFunction func;
	if (local_queue->TryDequeue(func)) {
		XAMP_LOG_D(logger_, "Pop local queue ({}).", local_queue->size());
		return func;
	}
	return std::nullopt;
}

std::optional<MoveOnlyFunction> TaskScheduler::TrySteal(StopToken const& stop_token, size_t i) {
	for (size_t n = 0; n != max_thread_; ++n) {
		if (stop_token.stop_requested()) {
			return std::nullopt;
		}

		const auto index = (i + n) % max_thread_;

		if (auto func = task_work_queues_.at(index)->TryDequeue()) {
			XAMP_LOG_D(logger_, "Steal other thread {} queue.", index);
			return func;
		}
	}
	return std::nullopt;
}

void TaskScheduler::SetWorkerThreadName(size_t i) {
	std::wostringstream stream;
	stream << String::ToStdWString(pool_name_) << L" Worker Thread(" << i << ")";
	SetThreadName(stream.str());
}

void TaskScheduler::CreateIdleThread() {
	JThread([this]() {
		constexpr std::chrono::milliseconds kIdleThreadCheckInterval(1000);
		constexpr std::chrono::milliseconds kIdleThreadTimeout(6000);

		Stopwatch stopwatch;

		while (!is_stopped_) {
			auto thread_size = 0;
			{
				std::lock_guard<FastMutex> lock(mutex_);
				thread_size = threads_.size();
			}

			const auto idle_thread_count = thread_size - running_thread_;

			if (idle_thread_count > last_idle_thread_count_ &&
				stopwatch.Elapsed<std::chrono::milliseconds>() >= kIdleThreadTimeout) {
				AddThread();
				last_idle_thread_count_ = idle_thread_count;
			}

			if (idle_thread_count < last_idle_thread_count_ &&
				stopwatch.Elapsed<std::chrono::milliseconds>() >= kIdleThreadTimeout) {
				RemoveThread();
				last_idle_thread_count_ = idle_thread_count;
			}

			if (idle_thread_count == thread_size) {
				stopwatch.Reset();
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(kIdleThreadCheckInterval));
		}
		}).detach();
}

void TaskScheduler::AddThread() {
	std::lock_guard<FastMutex> lock(mutex_);
	if (threads_.size() < max_thread_) {
		task_work_queues_.push_back(MakeAlign<WorkStealingTaskQueue>(kMaxWorkQueueSize));
		AddThread(threads_.size() + 1, thread_priority_);
		XAMP_LOG_I(logger_, "ThreadPoolExecutor add new thread, total: {}", threads_.size());
	}
}

void TaskScheduler::RemoveThread() {
	std::lock_guard<FastMutex> lock(mutex_);
	if (threads_.size() > min_thread_) {
		auto& thread = threads_.back();
		thread.request_stop();
		thread.join();
		threads_.pop_back();
		XAMP_LOG_I(logger_, "ThreadPoolExecutor remove thread, total: {}", threads_.size());
	}
}

void TaskScheduler::AddThread(size_t i, ThreadPriority priority) {
    threads_.emplace_back([i, this, priority](StopToken stop_token) mutable {
		// Avoid 64K Aliasing in L1 Cache (Intel hyper-threading)
		const auto L1_padding_buffer =
			MakeStackBuffer((std::min)(kInitL1CacheLineSize * i,
			kMaxL1CacheLineSize));

		SetWorkerThreadName(i);

		XAMP_LOG_D(logger_, "Worker Thread {} priority:{}.", i, priority);

		auto* local_queue = task_work_queues_[i].get();
		auto* policy = task_scheduler_policy_.get();
		auto steal_failure_count = 0;
		auto thread_id = GetCurrentThreadId();

		XAMP_LOG_D(logger_, "Worker Thread {} ({}) suspend.", thread_id, i);
		work_done_.count_down();

		start_clean_up_.wait();

#ifdef XAMP_OS_WIN
		SetThreadPriority(threads_.at(i), priority);
		SetThreadMitigation();
#endif

		XAMP_LOG_D(logger_, "Worker Thread {} ({}) resume.", thread_id, i);

		XAMP_LOG_D(logger_, "Worker Thread {} ({}) start.", thread_id, i);

		while (!is_stopped_ && !stop_token.stop_requested()) {
			auto task = TryLocalPop(local_queue);

			if (!task) {
				const auto steal_index = policy->ScheduleNext(i, task_work_queues_);
				if (steal_index != (std::numeric_limits<size_t>::max)()) {
					task = TrySteal(stop_token, steal_index);
				}

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
			(*task)();
			--running_thread_;
			XAMP_LOG_D(logger_, "Worker Thread {} ({}) execute finished.", i, thread_id);
		}

		XAMP_LOG_D(logger_, "Worker Thread {} is existed.", i);
        });
}

ThreadPoolExecutor::ThreadPoolExecutor(const std::string_view& pool_name, TaskSchedulerPolicy policy, TaskStealPolicy steal_policy, uint32_t max_thread, CpuAffinity affinity, ThreadPriority priority)
	: IThreadPoolExecutor(MakeAlign<ITaskScheduler, TaskScheduler>(policy, steal_policy, pool_name, (std::min)(max_thread, kMaxThread), affinity, priority)) {
}

ThreadPoolExecutor::ThreadPoolExecutor(const std::string_view& pool_name, uint32_t max_thread, CpuAffinity affinity, ThreadPriority priority)
	: IThreadPoolExecutor(MakeAlign<ITaskScheduler, TaskScheduler>(pool_name, (std::min)(max_thread, kMaxThread), affinity, priority)) {
}

ThreadPoolExecutor::~ThreadPoolExecutor() {
	Stop();
}

size_t ThreadPoolExecutor::GetThreadSize() const {
	return scheduler_->GetThreadSize();
}

void ThreadPoolExecutor::Stop() {
	scheduler_->Destroy();
}

}
