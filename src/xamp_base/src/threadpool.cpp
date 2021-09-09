#include <algorithm>
#include <base/logger.h>
#include <base/platform.h>
#include <base/threadpool.h>

namespace xamp::base {

inline auto kPopWaitTimeout = std::chrono::milliseconds(5);

TaskScheduler::TaskScheduler(size_t max_thread, int32_t core)
	: is_stopped_(false)
	, active_thread_(0)
	, core_(core)
	, index_(0)
	, max_thread_(max_thread)
	, pool_queue_(max_thread * 16) {
	logger_ = Logger::GetInstance().GetLogger(kThreadPoolLoggerName);
	try {
		for (size_t i = 0; i < max_thread_; ++i) {
			shared_queues_.push_back(MakeAlign<TaskQueue>(max_thread));
		}
		for (size_t i = 0; i < max_thread_; ++i) {
            AddThread(i);
		}
	}
	catch (...) {
		is_stopped_ = true;
		throw;
	}	
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
	XAMP_LOG_D(logger_, "TaskScheduler initial max thread:{} affinity:{}", max_thread, core);
#endif
}

TaskScheduler::~TaskScheduler() noexcept {
	Destroy();
}

void TaskScheduler::SubmitJob(Task&& task) {
	const auto i = index_++;

	for (size_t n = 0; n < max_thread_ * K; ++n) {
		const auto index = (i + n) % max_thread_;
		if (shared_queues_.at(index)->TryEnqueue(task)) {
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
			XAMP_LOG_D(logger_, "Enqueue thread {} queue.", index);
#endif
			return;
		}
	}

	if (!pool_queue_.TryEnqueue(task)) {
		throw LibrarySpecException("Thread pool was fulled.");
	}
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
	XAMP_LOG_D(logger_, "Enqueue pool queue.");
#endif
}

void TaskScheduler::SetAffinityMask(int32_t core) {
	for (size_t i = 0; i < max_thread_; ++i) {
		SetThreadAffinity(threads_.at(i), core);
	}
	core_ = core;
}

void TaskScheduler::Destroy() noexcept {
	is_stopped_ = true;

	for (size_t i = 0; i < max_thread_; ++i) {
		try {
			shared_queues_.at(i)->WakeupForShutdown();

			if (threads_.at(i).joinable()) {
				threads_.at(i).join();
			}
		}
		catch (...) {
		}
	}
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
	XAMP_LOG_D(logger_, "Thread pool was destory.");
#endif
}

size_t TaskScheduler::GetActiveThreadCount() const noexcept {
	return active_thread_;
}

std::optional<Task> TaskScheduler::TryPopPoolQueue() {	
	Task task;
	if (pool_queue_.Dequeue(task, kPopWaitTimeout)) {
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
		XAMP_LOG_D(logger_, "Pop pool thread queue.");
#endif
		return std::move(task);
	}
	return std::nullopt;
}

std::optional<Task> TaskScheduler::TryPopLocalQueue(size_t index) {
	Task task;
	if (shared_queues_.at(index)->Dequeue(task)) {
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
		XAMP_LOG_D(logger_, "Pop local thread queue.");
#endif
		return std::move(task);
	}
	return std::nullopt;
}

std::optional<Task> TaskScheduler::TrySteal(size_t i) {
	Task task;
	for (size_t n = 0; n != max_thread_ * K; ++n) {
		if (is_stopped_) {
			return std::nullopt;
		}

		const auto index = (i + n) % max_thread_;
		if (shared_queues_.at(index)->TryDequeue(task)) {
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
			XAMP_LOG_D(logger_, "Steal other thread {} queue.", index);
#endif
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
	stream << "Worker Thread(" << i << ").";
	SetThreadName(stream.str());
}

void TaskScheduler::AddThread(size_t i) {
	threads_.emplace_back([i, this]() mutable {		
		const auto allocate_stack_size = (std::min)(kInitL1CacheLineSize * i, 
			kMaxL1CacheLineSize);
		const auto L1_padding_buffer = MakeStackBuffer<uint8_t>(allocate_stack_size);
		auto thread_id = GetCurrentThreadId();
		
		SetWorkerThreadName(i);
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
		XAMP_LOG_D(logger_, "Worker Thread {} ({}) start.", thread_id, i);
#endif
		for (; !is_stopped_;) {
			auto task = TrySteal(i + 1);
			if (!task) {
				task = TryPopLocalQueue(i);

				if (!task) {
					task = TryPopPoolQueue();
				}

				if (!task) {
					// 如果連TryPopPoolQueue都會資料代表有經過等待. 就不切出CPU給其他的Thread.
					// std::this_thread::yield();
					continue;
				}
			}
			auto active_thread = ++active_thread_;
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
			XAMP_LOG_D(logger_, "Worker Thread {} ({}) weakup, active:{}.", i, thread_id, active_thread);
#endif
			try {
				(*task)();
			} catch (std::exception const& e) {
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
				XAMP_LOG_D(logger_, "Worker Thread {} got exception: {}", e.what());
#endif
			} catch (...) {
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
				XAMP_LOG_ERROR("unknown exception!");
#endif
			}
			--active_thread_;
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
			XAMP_LOG_D(logger_, "Worker Thread {} ({}) execute finished.", i, thread_id);
#endif
		}
		});

	if (core_ != -1) {
		SetThreadAffinity(threads_.at(i), core_);
	}
}

ThreadPool::ThreadPool()
	: scheduler_((std::min)(std::thread::hardware_concurrency(), kMaxThread)) {
}

size_t ThreadPool::GetActiveThreadCount() const noexcept {
	return scheduler_.GetActiveThreadCount();
}

void ThreadPool::Stop() {
	scheduler_.Destroy();
}

void ThreadPool::SetAffinityMask(int32_t core) {
	scheduler_.SetAffinityMask(core);
}

ThreadPool& ThreadPool::GetInstance() {
	static ThreadPool threadpool;
	return threadpool;
}

ThreadPool& ThreadPool::WASAPIThreadPool() {
	static ThreadPool wasapi_threadpool;
	return wasapi_threadpool;
}

}
