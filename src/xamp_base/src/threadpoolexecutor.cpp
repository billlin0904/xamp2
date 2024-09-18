#include <base/threadpoolexecutor.h>

#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/platform.h>
#include <base/rng.h>
#include <base/stopwatch.h>
#include <base/latch.h>
#include <base/crashhandler.h>

#include <algorithm>
#include <sstream>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
	constexpr std::chrono::milliseconds kSpinningTimeout = std::chrono::milliseconds(10);
	constexpr auto kDequeueTimeout = std::chrono::milliseconds(100);
	constexpr auto kSharedTaskQueueSize = 4096;
	constexpr auto kWorkStealingTaskQueueSize = 4096;
	constexpr auto kMaxWorkQueueSize = 65536;
	constexpr size_t kMaxThreadPoolSize = 8;

	bool IsCPUSupportHT() {
		int32_t reg[4]{ 0 };
#ifdef XAMP_OS_WIN
		::__cpuid(reg, 1); // CPUID function 1 gives processor features
#elif defined(XAMP_OS_MAC)
		__cpuid(1, reg[0], reg[1], reg[2], reg[3]);
#endif
		// Bit 28 of reg[3] (EDX) indicates support for Hyper-Threading
		return (reg[3] & (1 << 28)) != 0;
	}
}

TaskScheduler::TaskScheduler(const std::string_view& pool_name, size_t max_thread, const CpuAffinity& affinity, ThreadPriority priority)
	: TaskScheduler(TaskSchedulerPolicy::THREAD_LOCAL_RANDOM_POLICY, TaskStealPolicy::CONTINUATION_STEALING_POLICY, pool_name, max_thread, affinity, priority)  {
}

TaskScheduler::TaskScheduler(TaskSchedulerPolicy policy, TaskStealPolicy steal_policy, const std::string_view& pool_name, size_t max_thread, CpuAffinity affinity, ThreadPriority priority)
	: is_stopped_(false)
	, running_thread_(0)
	, max_thread_(std::max(max_thread, kMaxThreadPoolSize))
	, pool_name_(pool_name)
	, task_execute_flags_(max_thread_)
	, task_steal_policy_(MakeTaskStealPolicy(steal_policy))
	, task_scheduler_policy_(MakeTaskSchedulerPolicy(policy))
	, work_done_(static_cast<ptrdiff_t>(max_thread_))
	, start_clean_up_(1)
	, cpu_affinity_(affinity) {
	logger_ = XampLoggerFactory.GetLogger(pool_name);

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
	JThread([this, priority]() mutable {
		for (size_t i = 0; i < max_thread_; ++i) {
			if (cpu_affinity_.IsCoreUse(i)) {
				cpu_affinity_.SetAffinity(threads_.at(i));
				XAMP_LOG_D(logger_, "Worker Thread {} affinity:{}.", i, cpu_affinity_);
			}
		}
        XAMP_LOG_D(logger_, "Set ({}) Thread affinity, priority is success.", max_thread_);
		start_clean_up_.count_down();
		}).detach();

	XAMP_LOG_D(logger_,
		"TaskScheduler initial max thread:{} affinity:{} priority:{}",
		max_thread, cpu_affinity_, priority);
}

TaskScheduler::~TaskScheduler() {
	Destroy();
}

size_t TaskScheduler::GetThreadSize() const {
	return max_thread_;
}

void TaskScheduler::SubmitJob(MoveOnlyFunction&& task, ExecuteFlags flags) {
	auto* policy = task_scheduler_policy_.get();
	task_steal_policy_->SubmitJob(std::move(task),
		flags,
		max_thread_,
		task_pool_.get(),
		policy, 
		task_work_queues_,
		task_execute_flags_);
}

void TaskScheduler::Destroy() noexcept {
	if (!task_pool_ || threads_.empty()) {
		return;
	}

	XAMP_LOG_D(logger_, "Thread pool start destroy.");

	// Wait for all threads to finish
	is_stopped_ = true;

	// Wake up all threads
	task_pool_->WakeupForShutdown();

	// Wait for all threads to finish
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
	task_execute_flags_.clear();

	task_scheduler_policy_.reset();
    task_steal_policy_.reset();	

	XAMP_LOG_D(logger_, "Thread pool was destroy.");
}

std::optional<MoveOnlyFunction> TaskScheduler::TryDequeueSharedQueue(const StopToken& stop_token, std::chrono::milliseconds timeout) {
	if (stop_token.stop_requested()) {
		return std::nullopt;
	}

	MoveOnlyFunction func;
	// Wait for a task to be available
	if (task_pool_->Dequeue(func, timeout)) {
		XAMP_LOG_D(logger_, "Pop shared queue.");
		return func;
	}
	return std::nullopt;
}

std::optional<MoveOnlyFunction> TaskScheduler::TryDequeueSharedQueue(const StopToken& stop_token) {
	if (stop_token.stop_requested()) {
		return std::nullopt;
	}

	MoveOnlyFunction func;
	// Wait for a task to be available
    if (task_pool_->TryDequeue(func)) {
        XAMP_LOG_D(logger_, "Pop shared queue.");
		return func;
	}
	return std::nullopt;
}

std::optional<MoveOnlyFunction> TaskScheduler::TryLocalPop(const StopToken& stop_token, WorkStealingTaskQueue* local_queue) const {
	if (stop_token.stop_requested()) {
		return std::nullopt;
	}

	MoveOnlyFunction func;
	// Try to get a task from the local queue
	if (local_queue->TryDequeue(func)) {
		XAMP_LOG_D(logger_, "Pop local queue ({}).", local_queue->size());
		return func;
	}
	return std::nullopt;
}

std::optional<MoveOnlyFunction> TaskScheduler::TrySteal(const StopToken& stop_token, size_t i) {
	// Try to steal a task from another thread's queue
	// Note: The order in which we try the queues is important to prevent
	//       all threads from trying to steal from the same thread
	// 	 (which would lead to a deadlock)

	constexpr size_t kMaxAttempts = 100;
	size_t attempts = 0;

	for (size_t n = 0; attempts < kMaxAttempts && n != max_thread_; ++n, ++attempts) {
		if (stop_token.stop_requested()) {
			return std::nullopt;
		}

		const auto index = (i + n) % max_thread_;

		MoveOnlyFunction func;
		if (task_work_queues_.at(index)->TryDequeue(func)) {
			XAMP_LOG_D(logger_, "Steal other thread {} queue (found count: {}).", index, n);
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

void TaskScheduler::AddThread(size_t i, ThreadPriority priority) {	
    threads_.emplace_back([i, this, priority](const StopToken& stop_token) mutable {
		StackBufferPtr<std::byte> L1_padding_buffer;

		if (IsCPUSupportHT()) {
			// Avoid 64K Aliasing in L1 Cache (Intel hyper-threading)
			L1_padding_buffer =
				MakeStackBuffer((std::min)(kInitL1CacheLineSize * i,
					kMaxL1CacheLineSize));
		}

		XampCrashHandler.SetThreadExceptionHandlers();

		SetWorkerThreadName(i);

		XAMP_LOG_D(logger_, "Worker Thread {} priority:{}.", i, priority);

		auto* local_queue = task_work_queues_[i].get();
		auto* policy = task_scheduler_policy_.get();
		const auto thread_id = GetCurrentThreadId();

		XAMP_LOG_D(logger_, "Worker Thread {} ({}) suspend.", thread_id, i);
		work_done_.count_down();

#ifdef XAMP_OS_WIN
		SetThreadPriority(threads_.at(i), priority);
		SetThreadMitigation();
#endif

		start_clean_up_.wait();

		XAMP_LOG_D(logger_, "Worker Thread {} ({}) resume.", thread_id, i);
		XAMP_LOG_D(logger_, "Worker Thread {} ({}) start.", thread_id, i);
		
		Stopwatch spinning_watch;		

		// Main loop
		while (!is_stopped_ && !stop_token.stop_requested()) {
			// Try to get a task from the local queue
			auto task = TryLocalPop(stop_token, local_queue);
			auto steal_index = 0;

			// Try to get a task from the global queue
			if (!task) {
				steal_index = policy->ScheduleNext(i, task_work_queues_, task_execute_flags_);				
				if (steal_index != kInvalidScheduleIndex) {
					// Try to steal a task from another thread's queue
					task = TrySteal(stop_token, steal_index);
				}
			}

			// Try to get a task from the shared queue
			if (!task) {				
				if (spinning_watch.Elapsed() >= kSpinningTimeout) {
					task = TryDequeueSharedQueue(stop_token, kDequeueTimeout);					
				}
				else {
					task = TrySteal(stop_token, steal_index);
				}
			}

			// If no task was found, wait for a notification
			if (!task) {
				CpuRelax();
				continue;
			} else {
				spinning_watch.Reset();
			}

			auto running_thread = ++running_thread_;
			(*task)(stop_token);
			--running_thread_;

			task_execute_flags_[i] = ExecuteFlags::EXECUTE_NORMAL;
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

XAMP_BASE_NAMESPACE_END
