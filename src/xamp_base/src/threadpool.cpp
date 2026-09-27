#include <base/threadpool.h>

#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/platform.h>
#include <base/rng.h>
#include <base/crashhandler.h>

#include <algorithm>
#include <sstream>

XAMP_BASE_NAMESPACE_BEGIN

// Intel HT Technical User's Guide, p.23/p.27：
// 為各 worker 加上不同 private stack offset，避免 stack 區域變數剛好形成
// 64KB / 1MB aliasing pattern，降低 L1D 不必要的 cache-line eviction。
#define ENABLE_INTEL_CPU_AVOID_64KB_ALIASING 0

namespace {
	constexpr size_t kMaxAttempts = 100;
	constexpr auto kIdleWaitTimeout = std::chrono::milliseconds(1000);
	constexpr auto kSharedTaskQueueSize = 512;
	constexpr auto kMaxWorkQueueSize = 1024;
	constexpr size_t kMinThreadPoolSize = 1;

	thread_local struct CurrentTaskScheduler {
		size_t thread_index{ static_cast<size_t>(-1) };
		TaskScheduler* scheduler{ nullptr };
	} g_current_scheduler;

	bool isCurrentThreadInScheduler(const TaskScheduler* scheduler, size_t thread_index) {
		return g_current_scheduler.scheduler == scheduler
			&& g_current_scheduler.thread_index == thread_index;
	}
}

TaskScheduler::TaskScheduler(const std::string_view& name,
	size_t max_thread,
	size_t bulk_size,
	ThreadPriority priority)
	: is_stopped_(false)
	, running_thread_(0)
	, max_thread_(std::max(max_thread, kMinThreadPoolSize))
	, bulk_size_((std::max)(bulk_size, size_t{ 1 }))
	, name_(name)
	, task_execute_flags_(max_thread_)
	, enqueue_hint_{}
	, work_done_(static_cast<ptrdiff_t>(max_thread_))
	, start_clean_up_(1) {
	logger_ = XampLoggerFactory.getLogger(name);
	//logger_->setLevel(LogLevel::LOG_LEVEL_DEBUG);

	try {
		shared_queue_ = makeAlign<SharedTaskQueue>(kSharedTaskQueueSize);
		task_work_queues_.resize(max_thread_);

		for (size_t i = 0; i < max_thread_; ++i) {
            addThread(i, priority);
		}
	}
	catch (...) {
		is_stopped_ = true;
		throw;
	}

	work_done_.wait();
	XAMP_LOG_D(logger_, "Set ({}) Thread affinity, priority is success.", max_thread_);
	start_clean_up_.count_down();

	XAMP_LOG_D(logger_,
		"TaskScheduler initial max thread:{} priority:{}",
		max_thread, enumToString(priority));
}

TaskScheduler::~TaskScheduler() {
	destroy();
}

void IThreadPool::resumeCoroutine(SubmitPolicy policy,
	ExecuteFlags flags,
	std::coroutine_handle<> handle) {
	scheduler_->submit(
		[handle](const std::stop_token&) mutable {
			handle.resume();
		},
		flags,
		policy);
}

void IThreadPool::submitCoroutine(SubmitPolicy policy,
	ExecuteFlags flags,
	Task task) {
	scheduler_->submit(std::move(task), flags, policy);
}

size_t TaskScheduler::getThreadSize() const {
	return max_thread_;
}

void TaskScheduler::destroy() {
	if (!shared_queue_ || threads_.empty()) {
		return;
	}

	XAMP_LOG_D(logger_, "Thread pool start destroy.");

	// Wait for all threads to finish
	is_stopped_ = true;

	for (auto& t : threads_)
		t.request_stop();

	// Wake up all threads
	notifyAllWorkers();
	shared_queue_->wakeup_for_shutdown();

	// Wait for all threads to finish
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

	shared_queue_.reset();
	threads_.clear();
	task_work_queues_.clear();
	task_execute_flags_.clear();

	XAMP_LOG_D(logger_, "Thread pool was destroy.");
}

size_t TaskScheduler::tryDequeueSharedQueue(std::vector<Task>& tasks,
	const std::stop_token& stop_token,
	std::chrono::milliseconds timeout) {
	if (!stop_token.stop_requested()) {
		if (shared_queue_->dequeue(tasks[0], timeout)) {
#ifdef _DEBUG
			XAMP_EXPECTS(tasks[0]);
#endif
			size_t task_size = 1;
			while (task_size < tasks.size() && shared_queue_->try_dequeue(tasks[task_size])) {
				++task_size;
			}
			return task_size;
		}
	}
	return 0;
}

size_t TaskScheduler::tryDequeueSharedQueue(std::vector<Task>& tasks,
	const std::stop_token& stop_token) {
	if (!stop_token.stop_requested()) {
		if (shared_queue_->try_dequeue(tasks[0])) {
#ifdef _DEBUG
			XAMP_EXPECTS(tasks[0]);
#endif
			size_t task_size = 1;
			while (task_size < tasks.size() && shared_queue_->try_dequeue(tasks[task_size])) {
				++task_size;
			}
			return task_size;
		}
	}
	return 0;
}

bool TaskScheduler::isLongRunning(size_t index) const {
	return task_execute_flags_[index].value.load(std::memory_order_acquire)
	== ExecuteFlags::EXECUTE_LONG_RUNNING;
}

size_t TaskScheduler::tryLocalPop(std::vector<Task>& tasks,
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

size_t TaskScheduler::trySteal(std::vector<Task>& tasks,
	const std::stop_token& stop_token,
	size_t random_start,
	size_t current_thread_index) {

	if (!stop_token.stop_requested()) {
		for (size_t attempts = 0; attempts < kMaxAttempts; ++attempts) {
			size_t random_index = (random_start + attempts) % max_thread_;
			if (random_index == current_thread_index) {
				continue;
			}

			if (isLongRunning(random_index)) {
				continue;
			}

			auto* queue = task_work_queues_[random_index].get();
			if (queue == nullptr) {
				continue;
			}

			auto size = queue->try_dequeue_bulk(
				tasks.begin(), tasks.size());
			if (size > 0) {
				return size;
			}
		}
	}
	return 0;
}

void TaskScheduler::submit(Task task, ExecuteFlags flags, SubmitPolicy policy) {
	// Enqueue policy:
	// - LOCAL keeps worker-originated fire-and-forget tasks on the current
	//   worker queue for cache locality and lower submit overhead.
	// - External LOCAL submissions fall back to the round-robin path below.
	// - FORK avoids the current worker queue so nested spawn/wait patterns do
	//   not depend on the worker that is already blocked waiting for the result.
	// - NORMAL uses round-robin probing to spread work across worker queues.

	const auto probe_count = (std::min)(max_thread_, kMaxAttempts);

	auto enqueue_to_worker = [this, flags, &task](size_t index) {
		auto* task_queue = task_work_queues_[index].get();
		if (task_queue == nullptr || !task_queue->try_enqueue(std::move(task))) {
			return false;
		}

		task_execute_flags_[index].value.store(flags, std::memory_order_release);
		XAMP_LOG_D(logger_, "TaskScheduler::submit() enqueue task to local queue.");
		notifyWorkAvailable();
		return true;
	};

	if (policy == SubmitPolicy::SUBMIT_POLICY_LOCAL) {
		const bool is_current_worker =
			g_current_scheduler.scheduler == this &&
			g_current_scheduler.thread_index < max_thread_;

		if (is_current_worker && enqueue_to_worker(g_current_scheduler.thread_index)) {
			return;
		}
	}

	// round-robin enqueue hint to reduce contention on the same thread's local queue.
	const auto start_index = enqueue_hint_.value.fetch_add(1, std::memory_order_relaxed);

	for (size_t attempts = 0; attempts < probe_count; ++attempts) {
		size_t random_index = (start_index + attempts) % max_thread_;

		if (policy == SubmitPolicy::SUBMIT_POLICY_FORK) {
			// Avoid enqueue to the current worker thread's local queue to prevent deadlock in nested spawn/wait patterns.
			if (isCurrentThreadInScheduler(this, random_index)) {
				continue;
			}
		}

		if (isLongRunning(random_index)) {
			continue;
		}

		if (enqueue_to_worker(random_index)) {
			return;
		}
	}

	XAMP_LOG_D(logger_, "TaskScheduler::submit() failed to enqueue task. Enqueue to shared queue.");
	shared_queue_->enqueue(std::move(task));
	notifyWorkAvailable();
}

void TaskScheduler::setWorkerThreadName(size_t i) {
	std::wostringstream stream;
	stream << String::toStdWString(name_) << L" Worker Thread(" << i << ")";
	setThreadName(stream.str());
}

void TaskScheduler::execute(std::vector<Task>& tasks,
	size_t task_size,
	size_t current_index,
	const std::stop_token& stop_token) {
	for (size_t i = 0; i < task_size; ++i) {
		auto running_thread = ++running_thread_;
		std::invoke(tasks[i], stop_token);
		tasks[i] = nullptr;
		--running_thread_;
	}
	task_execute_flags_[current_index].value.store(ExecuteFlags::EXECUTE_NORMAL, std::memory_order_release);
}

void TaskScheduler::notifyWorkAvailable() {
	work_epoch_.value.fetch_add(1, std::memory_order_release);
	idle_cv_.notify_one();
	// Notify the shared queue to wake up one waiting thread if any.	 
	shared_queue_->wakeup();
}

void TaskScheduler::notifyAllWorkers() {
	work_epoch_.value.fetch_add(1, std::memory_order_release);
	idle_cv_.notify_all();
}

void TaskScheduler::waitForWork(uint32_t observed_epoch,
	const std::stop_token& stop_token) {
	if (is_stopped_.load(std::memory_order_acquire) || stop_token.stop_requested()) {
		return;
	}

	std::unique_lock lock{ idle_mutex_ };
	idle_cv_.wait_for(lock, kIdleWaitTimeout, [this, observed_epoch, &stop_token] {
		return is_stopped_.load(std::memory_order_acquire)
			|| stop_token.stop_requested()
			|| work_epoch_.value.load(std::memory_order_acquire) != observed_epoch;
		});
}

void TaskScheduler::addThread(size_t i, ThreadPriority priority) {
	constexpr size_t kExternalProducerReserve = 4;
	task_work_queues_[i] = makeAlign<WorkStealingTaskQueue>(
		kMaxWorkQueueSize,
		1,
		max_thread_ + kExternalProducerReserve);
	auto* local_work_queue = task_work_queues_[i].get();

    threads_.emplace_back([i, this, local_work_queue, priority](const auto& stop_token) mutable {
#if ENABLE_INTEL_CPU_AVOID_64KB_ALIASING		
		constexpr size_t kStackAliasOffsetStride = 128UL;
		constexpr size_t kMaxStackAliasOffset = 64 * 1024UL;
		const auto allocate_stack_size =
			(std::min)(kStackAliasOffsetStride * (i + 1), kMaxStackAliasOffset);
		StackBuffer<std::byte> stack_aliasing_offset(stackAlloc(allocate_stack_size));
#endif
		const auto thread_id = getCurrentThreadId();
		XAMP_LOG_D(logger_, "Worker Thread {} ({}) suspend.", thread_id, i);
		work_done_.count_down();

		XampCrashHandler.setThreadExceptionHandlers();
		setWorkerThreadName(i);

		g_current_scheduler = CurrentTaskScheduler{ i, this };

		setCurrentThreadPriority(priority);
#ifdef XAMP_OS_WIN
		setCurrentThreadMitigation();
#endif

		start_clean_up_.wait();

		XAMP_LOG_D(logger_, "Worker Thread {} ({}) resume.", thread_id, i);
		XAMP_LOG_D(logger_, "Worker Thread {} ({}) start.", thread_id, i);

		std::vector<Task> tasks(bulk_size_);

		auto try_get_task = [&tasks, &stop_token, local_work_queue, this, i] {
			auto task_size = tryLocalPop(tasks, stop_token, local_work_queue);
			if (!task_size) {
				task_size = tryDequeueSharedQueue(tasks, stop_token);
				if (!task_size) {
					auto& prng = PRNG::getThreadLocal();
					task_size = trySteal(tasks, stop_token, prng() % max_thread_, i);
				}
			}
			return task_size;
		};

		constexpr auto kMaxSpinCount = 64;
		size_t spin_count = 0;
		while (!is_stopped_ && !stop_token.stop_requested()) {
			auto task_size = try_get_task();

			if (task_size > 0) {
				execute(tasks, task_size, i, stop_token);
				continue;
			}

			const auto observed_epoch = work_epoch_.value.load(std::memory_order_acquire);
			task_size = try_get_task();
			if (task_size > 0) {
				execute(tasks, task_size, i, stop_token);
				continue;
			}

			++spin_count;
			if (spin_count >= kMaxSpinCount) {
				waitForWork(observed_epoch, stop_token);
				spin_count = 0;
			}
			else {
				cpuRelax();
			}
		}

		XAMP_LOG_D(logger_, "Worker Thread {} is exited.", i);
        });
}

ThreadPool::ThreadPool(const std::string_view& name,
	uint32_t max_thread,
	size_t bulk_size,
	ThreadPriority priority)
	: IThreadPool(makeAlign<ITaskScheduler, TaskScheduler>(
		name,
		max_thread,
		bulk_size,
		priority)) {
}

ThreadPool::~ThreadPool() {
	stop();
}

size_t ThreadPool::getThreadSize() const {
	return scheduler_->getThreadSize();
}

void ThreadPool::stop() {
	scheduler_->destroy();
}

XAMP_BASE_NAMESPACE_END
