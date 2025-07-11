#include <base/threadpoolexecutor.h>

#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/platform.h>
#include <base/rng.h>
#include <base/stopwatch.h>
#include <base/crashhandler.h>

#include <algorithm>
#include <sstream>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
	constexpr size_t kMaxAttempts = 100;
	constexpr auto kCheckEwaInterval = std::chrono::milliseconds(500);
	constexpr auto kDequeueTimeout = std::chrono::milliseconds(100);
	constexpr auto kSharedTaskQueueSize = 4096;
	constexpr auto kMaxWorkQueueSize = 65536;
	constexpr size_t kMinThreadPoolSize = 1;
}

TaskScheduler::TaskScheduler(const std::string_view& name,
	size_t max_thread,
	size_t bulk_size,
	ThreadPriority priority)
	: is_stopped_(false)
	, running_thread_(0)
	, max_thread_(std::max(max_thread, kMinThreadPoolSize))
	, bulk_size_(bulk_size)
	, name_(name)
	, task_execute_flags_(max_thread_)
	, attempt_count_(max_thread_)
	, success_count_(max_thread_)
	, work_done_(static_cast<ptrdiff_t>(max_thread_))
	, start_clean_up_(1) {
	logger_ = XampLoggerFactory.GetLogger(name);

	try {
		task_pool_ = MakeAlign<SharedTaskQueue>(kSharedTaskQueueSize);
		task_work_queues_.resize(max_thread_);
		
		for (size_t i = 0; i < max_thread_; ++i) {
			task_work_queues_[i] = MakeAlign<WorkStealingTaskQueue>(kMaxWorkQueueSize);
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

	std::jthread([this]() mutable {
        XAMP_LOG_D(logger_, "Set ({}) Thread affinity, priority is success.", max_thread_);
		start_clean_up_.count_down();
		}).detach();

	XAMP_LOG_D(logger_,
		"TaskScheduler initial max thread:{} priority:{}",
		max_thread, priority);
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

	for (auto& t : threads_)
		t.request_stop();

	// Wake up all threads
	task_pool_->wakeup_for_shutdown();

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

size_t TaskScheduler::TryDequeueSharedQueue(std::vector<MoveOnlyFunction>& tasks,
	const std::stop_token& stop_token, 
	std::chrono::milliseconds timeout) {
	if (!stop_token.stop_requested()) {
		if (task_pool_->dequeue(tasks[0], timeout)) {
#ifdef _DEBUG
			XAMP_EXPECTS(tasks[0]);
#endif
			return 1;
		}
	}
	return 0;
}

size_t TaskScheduler::TryDequeueSharedQueue(std::vector<MoveOnlyFunction>& tasks,
	const std::stop_token& stop_token) {
	if (!stop_token.stop_requested()) {
		if (task_pool_->try_dequeue(tasks[0])) {
#ifdef _DEBUG
			XAMP_EXPECTS(tasks[0]);
#endif
			return 1;
		}
	}
	return 0;
}

bool TaskScheduler::IsLongRunning(size_t index) const {
	return task_execute_flags_[index].value.load(std::memory_order_acquire)
	== ExecuteFlags::EXECUTE_LONG_RUNNING;
}

size_t TaskScheduler::TryLocalPop(std::vector<MoveOnlyFunction>& tasks,
	const std::stop_token& stop_token,
	WorkStealingTaskQueue* local_queue) const {
	if (!stop_token.stop_requested()) {
		auto size = local_queue->try_dequeue_bulk(tasks.begin(),
			tasks.size());
		if (size > 0) {
			return size;
		}
	}	
	return 0;
}

size_t TaskScheduler::TrySteal(std::vector<MoveOnlyFunction>& tasks,
	const std::stop_token& stop_token,
	size_t random_start, 
	size_t current_thread_index) {

	if (!stop_token.stop_requested()) {
		auto& attempt_count = attempt_count_[current_thread_index];

		for (size_t attempts = 0; attempts < kMaxAttempts; ++attempts) {
			attempt_count.value.fetch_add(1, std::memory_order_relaxed);

			size_t random_index = (random_start + attempts) % max_thread_;
			if (random_index == current_thread_index) {
				continue;
			}

			if (IsLongRunning(random_index)) {
				continue;
			}

			auto& queue = task_work_queues_.at(random_index);
			auto size = queue->try_dequeue_bulk(
				tasks.begin(), tasks.size());
			if (size > 0) {
				success_count_[current_thread_index].value.fetch_add(1,
					std::memory_order_relaxed);
				return size;
			}
		}
	}
	return 0;
}

void TaskScheduler::SubmitJob(MoveOnlyFunction task, ExecuteFlags flags) {
	XAMP_NO_TLS_GUARDS thread_local PRNG prng;
	size_t random_start = prng() % max_thread_;
	for (size_t attempts = 0; attempts < kMaxAttempts; ++attempts) {
		size_t random_index = (random_start + attempts) % max_thread_;

		if (!IsLongRunning(random_index)) {
			auto& task_queue = task_work_queues_[random_index];
			if (task_queue->try_enqueue(std::move(task))) {
				task_execute_flags_[random_index].value = flags;
				XAMP_LOG_D(logger_, "TaskScheduler::SubmitJob() enqueue task to local queue.");
				return;
			}
		}
	}

	XAMP_LOG_D(logger_, "TaskScheduler::SubmitJob() failed to enqueue task. Enqueue to shared queue.");
	task_pool_->enqueue(task);
}

void TaskScheduler::SetWorkerThreadName(size_t i) {
	std::wostringstream stream;
	stream << String::ToStdWString(name_) << L" Worker Thread(" << i << ")";
	SetThreadName(stream.str());
}

void TaskScheduler::Execute(std::vector<MoveOnlyFunction>& tasks,
	size_t task_size,
	size_t current_index,
	const std::stop_token& stop_token) {
	for (size_t i = 0; i < task_size; ++i) {
		auto running_thread = ++running_thread_;
		try {
			std::invoke(tasks[i], stop_token);
			XAMP_LOG_D(logger_, "Execute running {} task", running_thread);
		}
		catch (const std::exception& e) {
			XAMP_LOG_E(logger_, "Execute running {} task failed. Exception:{}", running_thread, e.what());
		}
		catch (...) {
			XAMP_LOG_E(logger_, "Execute running {} task failed. Unknown exception.", running_thread);
		}
		--running_thread_;
	}
	task_execute_flags_[current_index].value = ExecuteFlags::EXECUTE_NORMAL;
}

void TaskScheduler::AddThread(size_t i, ThreadPriority priority) {
    threads_.emplace_back([i, this, priority](const auto& stop_token) mutable {
		auto * local_work_queue = task_work_queues_[i].get();

		XAMP_NO_TLS_GUARDS thread_local PRNG prng;
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

		ExponentialMovingAverage ema;

		Stopwatch check_ewa_watch;
		std::vector<MoveOnlyFunction> tasks(bulk_size_);

		while (!is_stopped_ && !stop_token.stop_requested()) {
			auto task_size = TryLocalPop(tasks, stop_token, local_work_queue);
			if (!task_size) {
				double success_rate = ema.GetValue();
				size_t random_start = prng() % max_thread_;
				if (success_rate < 0.1) {
					task_size = TryDequeueSharedQueue(tasks, stop_token, kDequeueTimeout);
					if (!task_size) {
						task_size = TrySteal(tasks, stop_token, random_start, i);
					}
				}
				else {
					task_size = TrySteal(tasks, stop_token, random_start, i);
				}
			}

			if (task_size > 0) {
				Execute(tasks, task_size, i, stop_token);
			} else {
				CpuRelax();
			}

			if (check_ewa_watch.Elapsed<std::chrono::milliseconds>() >= kCheckEwaInterval) {
				size_t attempts = attempt_count_[i].value.exchange(0);
				size_t successes = success_count_[i].value.exchange(0);
				if (attempts > 0) {
					double rate = static_cast<double>(successes) / attempts;
					ema.Update(rate);
					if (rate > 0.0) {
						XAMP_LOG_T(logger_, "Thread({}) Update ETA:{:.4f}", i, rate);
					}
				}
				check_ewa_watch.Reset();
			}
		}

		XAMP_LOG_D(logger_, "Worker Thread {} is exited.", i);
        });
}

ThreadPoolExecutor::ThreadPoolExecutor(const std::string_view& name,
	uint32_t max_thread,
	size_t bulk_size,
	ThreadPriority priority)
	: IThreadPoolExecutor(MakeAlign<ITaskScheduler, TaskScheduler>(
		name,
		max_thread,
		bulk_size, 
		priority)) {
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
