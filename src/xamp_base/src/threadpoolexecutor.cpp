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
	constexpr size_t kMaxAttempts = 100;
	constexpr auto kSpinningTimeout = std::chrono::milliseconds(500);
	constexpr auto kDequeueTimeout = std::chrono::milliseconds(10);
	constexpr auto kSharedTaskQueueSize = 4096;
	constexpr auto kMaxWorkQueueSize = 65536;
	constexpr size_t kMinThreadPoolSize = 1;

	/*
	 * Avoid 64k Alias conflicts.
	*/
	constexpr size_t kInitL1CacheLineSize{ 64 * 1024 };
	constexpr size_t kMaxL1CacheLineSize{ 1 * 1024 * 1024 };

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

TaskScheduler::TaskScheduler(const std::string_view& pool_name, size_t max_thread, const CpuAffinity &affinity, ThreadPriority priority)
	: is_stopped_(false)
	, running_thread_(0)
	, max_thread_(std::max(max_thread, kMinThreadPoolSize))
	, pool_name_(pool_name)
	, task_execute_flags_(max_thread_)
	, work_done_(static_cast<ptrdiff_t>(max_thread_))
	, start_clean_up_(1)
	, cpu_affinity_(affinity) {
	logger_ = XampLoggerFactory.GetLogger(pool_name);

	try {
		task_pool_ = MakeAlign<SharedTaskQueue>(kSharedTaskQueueSize);
		task_work_queues_.resize(max_thread_);
		for (size_t i = 0; i < max_thread_; ++i) {
            AddThread(i, priority);
		}
	}
	catch (...) {
		is_stopped_ = true;
		throw;
	}

	work_done_.wait();

	JThread([this]() mutable {
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

	XAMP_LOG_D(logger_, "Thread pool was destroy.");
}

MoveOnlyFunction TaskScheduler::TryDequeueSharedQueue(const StopToken& stop_token, std::chrono::milliseconds timeout) {
	if (stop_token.stop_requested()) {
		return {};
	}

	MoveOnlyFunction func;
	// Wait for a task to be available
	if (task_pool_->Dequeue(func, timeout)) {
		return func;
	}
	return {};
}

MoveOnlyFunction TaskScheduler::TryDequeueSharedQueue(const StopToken& stop_token) {
	if (stop_token.stop_requested()) {
		return {};
	}

	MoveOnlyFunction func;
	// Wait for a task to be available
    if (task_pool_->TryDequeue(func)) {
		return func;
	}
	return {};
}

MoveOnlyFunction TaskScheduler::TryLocalPop(const StopToken& stop_token, WorkStealingTaskQueue* local_queue) const {
	if (stop_token.stop_requested()) {
		return {};
	}

	MoveOnlyFunction func;
	//if (local_queue->TryDequeue(func)) {
	if (local_queue->try_dequeue(func)) {
		return func;
	}
	return {};
}

void TaskScheduler::SubmitJob(MoveOnlyFunction&& task, ExecuteFlags flags) {
	XAMP_NO_TLS_GUARDS thread_local PRNG prng;
	size_t random_start = prng() % max_thread_;

	for (size_t attempts = 0; attempts < kMaxAttempts; ++attempts) {
		size_t random_index = (random_start + attempts) % max_thread_;

		if (task_execute_flags_[random_index].load(std::memory_order_acquire) != ExecuteFlags::EXECUTE_LONG_RUNNING) {
			//if (task_work_queues_.at(random_index)->TryEnqueue(std::move(task))) {
			if (task_work_queues_.at(random_index)->try_enqueue(std::move(task))) {
				task_execute_flags_[random_index] = flags;
				return;
			}
		}
	}

	task_pool_->Enqueue(task);
}

MoveOnlyFunction TaskScheduler::TrySteal(const StopToken& stop_token, size_t random_start, size_t current_thread_index) {
	if (stop_token.stop_requested()) {
		return {};
	}
	
	for (size_t attempts = 0;  attempts < kMaxAttempts; ++attempts) {
		size_t random_index = (random_start + attempts) % max_thread_;
		if (random_index == current_thread_index) {
			continue;
		}

		if (task_execute_flags_[random_index].load(std::memory_order_acquire) != ExecuteFlags::EXECUTE_LONG_RUNNING) {
			MoveOnlyFunction task;
			//if (task_work_queues_.at(random_index)->TryDequeue(task)) {
			if (task_work_queues_.at(random_index)->try_dequeue(task)) {
				return task;
			}
		}
	}
	return {};
}

void TaskScheduler::SetWorkerThreadName(size_t i) {
	std::wostringstream stream;
	stream << String::ToStdWString(pool_name_) << L" Worker Thread(" << i << ")";
	SetThreadName(stream.str());
}

void TaskScheduler::AddThread(size_t i, ThreadPriority priority) {	
    threads_.emplace_back([i, this, priority](const StopToken& stop_token) mutable {
		StackBuffer<std::byte> L1_padding_buffer;

		if (IsCPUSupportHT()) {
			// Avoid 64K Aliasing in L1 Cache (Intel hyper-threading)
			L1_padding_buffer =
				MakeStackBuffer((std::min)(kInitL1CacheLineSize * i,
					kMaxL1CacheLineSize));
		}

		XAMP_NO_TLS_GUARDS thread_local PRNG prng;
		XAMP_NO_TLS_GUARDS thread_local WorkStealingTaskQueue task_local_queue(kMaxWorkQueueSize);
		auto local_queue = &task_local_queue;
		task_work_queues_[i] = local_queue;
		Stopwatch spinning_watch;

		XampCrashHandler.SetThreadExceptionHandlers();

		SetWorkerThreadName(i);

		XAMP_LOG_D(logger_, "Worker Thread {} priority:{}.", i, priority);

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

		// Main loop
		while (!is_stopped_ && !stop_token.stop_requested()) {
			auto task = TryLocalPop(stop_token, local_queue);

			if (!task) {
				size_t random_start = prng() % max_thread_;
				task = TrySteal(stop_token, random_start, i);

				if (!task && spinning_watch.Elapsed() >= kSpinningTimeout) {
					// Implement backoff by adjusting how frequently we check the shared queue
					static int backoff_attempts = 1;
					for (int attempt = 0; attempt < backoff_attempts; ++attempt) {
						task = TryDequeueSharedQueue(stop_token, kDequeueTimeout);
						if (task) {
							break; // Exit early if a task is found
						}
						CpuRelax(); // Small CPU relaxation between attempts
					}

					// Gradually increase the number of attempts if no task was found
					if (!task) {
						backoff_attempts = std::min(backoff_attempts * 2, 32); // Max backoff attempts
					}
					else {
						backoff_attempts = 1; // Reset backoff when a task is found
					}
				}
			}

			if (!task) {
				CpuRelax();
				continue;
			}

			spinning_watch.Reset();

			auto running_thread = ++running_thread_;
			std::invoke(task, stop_token);
			--running_thread_;

			task_execute_flags_[i] = ExecuteFlags::EXECUTE_NORMAL;
		}

		XAMP_LOG_D(logger_, "Worker Thread {} is existed.", i);
        });
}

ThreadPoolExecutor::ThreadPoolExecutor(const std::string_view& pool_name, uint32_t max_thread, const CpuAffinity &affinity, ThreadPriority priority)
	: IThreadPoolExecutor(MakeAlign<ITaskScheduler, TaskScheduler>(pool_name,max_thread, affinity, priority)) {
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
