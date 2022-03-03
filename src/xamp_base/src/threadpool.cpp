#include <algorithm>
#include <base/logger.h>
#include <base/platform.h>
#include <base/threadpool.h>

namespace xamp::base {

inline constexpr auto kPopWaitTimeout = std::chrono::milliseconds(5);

TaskScheduler::TaskScheduler(const std::string_view& pool_name, size_t max_thread, int32_t affinity, ThreadPriority priority)
	: is_stopped_(false)
	, running_thread_(0)
	, index_(0)
	, max_thread_(max_thread)
	, pool_queue_(max_thread * 16) {
	logger_ = Logger::GetInstance().GetLogger(pool_name.data());
	try {
		for (size_t i = 0; i < max_thread_; ++i) {
			shared_queues_.push_back(MakeAlign<TaskQueue>(max_thread));
		}
		for (size_t i = 0; i < max_thread_; ++i) {
            AddThread(i, affinity, priority);
		}
	}
	catch (...) {
		is_stopped_ = true;
		throw;
	}	
	XAMP_LOG_D(logger_, "TaskScheduler initial max thread:{} affinity:{}", max_thread, affinity);
}

TaskScheduler::~TaskScheduler() noexcept {
	Destroy();
}

void TaskScheduler::SubmitJob(Task&& task) {
	const auto i = index_++;

	for (size_t n = 0; n < max_thread_ * K; ++n) {
		const auto index = (i + n) % max_thread_;
		XAMP_LIKELY(shared_queues_.at(index)->TryEnqueue(task)) {
			XAMP_LOG_D(logger_, "Enqueue thread {} queue.", index);
			return;
		}
	}

	if (!pool_queue_.TryEnqueue(task)) {
		throw LibrarySpecException("Thread pool was fulled.");
	}
	XAMP_LOG_D(logger_, "Enqueue pool queue.");
}

void TaskScheduler::Destroy() noexcept {
	if (shared_queues_.empty() || threads_.empty()) {
		return;
	}

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

	XAMP_LOG_D(logger_, "Thread pool was destory.");
	shared_queues_.clear();
	threads_.clear();
}

std::optional<Task> TaskScheduler::TryPopLocalQueue(size_t index) {
	Task task;
    if (shared_queues_.at(index)->Dequeue(task)) {
		XAMP_LOG_D(logger_, "Pop local thread queue.");
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
		XAMP_LIKELY(shared_queues_.at(index)->TryDequeue(task)) {
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
	stream << "Worker Thread(" << i << ").";
	SetThreadName(stream.str());
}

void TaskScheduler::AddThread(size_t i, int32_t affinity, ThreadPriority priority) {
	threads_.emplace_back([i, this]() mutable {
		// Avoid 64K Aliasing in L1 Cache (Intel hyper-threading)
		const auto allocate_stack_size = (std::min)(kInitL1CacheLineSize * i,
			kMaxL1CacheLineSize);
		const auto L1_padding_buffer = MakeStackBuffer<uint8_t>(allocate_stack_size);
		auto thread_id = GetCurrentThreadId();
		SetWorkerThreadName(i);
		XAMP_LOG_D(logger_, "Worker Thread {} ({}) start.", thread_id, i);
		while (!is_stopped_) {
			auto task = TrySteal(i + 1);
			if (!task) {
				task = TryPopLocalQueue(i);

				if (!task) {
					// 如果連TryPopLocalQueue都會資料代表有經過等待. 就不切出CPU給其他的Thread.
					std::this_thread::yield();
					continue;
				}
			}
			auto running_thread = ++running_thread_;
			XAMP_LOG_D(logger_, "Worker Thread {} ({}) weakup, running:{}", i, thread_id, running_thread);
			try {
				(*task)(i);
			} catch (std::exception const& e) {
				XAMP_LOG_D(logger_, "Worker Thread {} got exception: {}", e.what());
			} catch (...) {
				XAMP_LOG_ERROR("unknown exception!");
			}
			--running_thread_;
			XAMP_LOG_D(logger_, "Worker Thread {} ({}) execute finished.", i, thread_id);
		}
		});

	if (affinity != -1) {
		SetThreadAffinity(threads_.at(i), affinity);
	}
	SetThreadPriority(threads_.at(i), priority);
}

ThreadPool::ThreadPool(const std::string_view& pool_name, uint32_t max_thread, int32_t affinity, ThreadPriority priority)
	: IThreadPool(MakeAlign<ITaskScheduler, TaskScheduler>(pool_name, (std::min)(max_thread, kMaxThread), affinity, priority)) {
}

ThreadPool::~ThreadPool() {
	Stop();
}

void ThreadPool::Stop() {
	scheduler_->Destroy();
}

}
