#include <algorithm>
#include <base/logger.h>
#include <base/platform.h>
#include <base/threadpool.h>

namespace xamp::base {

inline constexpr auto kDefaultTimeout = std::chrono::milliseconds(100);
inline constexpr auto kSharedTaskQueueSize = 1024;

TaskScheduler::TaskScheduler(const std::string_view& pool_name, size_t max_thread, int32_t affinity, ThreadPriority priority)
	: is_stopped_(false)
	, running_thread_(0)
	, index_(0)
	, max_thread_(max_thread) {
	logger_ = Logger::GetInstance().GetLogger(pool_name.data());
	try {
		shared_queues_ = MakeAlign<SharedTaskQueue>(kSharedTaskQueueSize);

		for (size_t i = 0; i < max_thread_; ++i) {
			workstealing_queue_list_.push_back(MakeAlign<WorkStealingTaskQueue>(max_thread_ * 32));
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

void TaskScheduler::SubmitJob(Task&& task) {
	const auto i = index_++;

	for (size_t n = 0; n < max_thread_ * K; ++n) {
		const auto index = (i + n) % max_thread_;
		auto& queue = workstealing_queue_list_.at(index);
		if (queue->TryEnqueue(task)) {
			XAMP_LOG_D(logger_, "Enqueue thread {} local queue ({}).", index, queue->size());
			return;
		}
	}

	XAMP_LOG_D(logger_, "Enqueue shared queue.");
	shared_queues_->Enqueue(task);
}

void TaskScheduler::Destroy() noexcept {
	if (!shared_queues_ || threads_.empty()) {
		return;
	}

	is_stopped_ = true;
	shared_queues_->WakeupForShutdown();

	for (size_t i = 0; i < max_thread_; ++i) {
		try {
			if (threads_.at(i).joinable()) {
				threads_.at(i).join();
			}
		}
		catch (...) {
		}
	}

	XAMP_LOG_D(logger_, "Thread pool was destory.");
	shared_queues_.reset();
	threads_.clear();
	workstealing_queue_list_.clear();
}

std::optional<Task> TaskScheduler::TryDequeueSharedQueue(std::chrono::milliseconds timeout) {
	Task task;
	if (shared_queues_->Dequeue(task, timeout)) {
		XAMP_LOG_D(logger_, "Pop shared queue.");
		return std::move(task);
	}
	return std::nullopt;
}

std::optional<Task> TaskScheduler::TryDequeueSharedQueue() {
	Task task;
    if (shared_queues_->TryDequeue(task)) {
		XAMP_LOG_D(logger_, "Pop shared queue.");
		return std::move(task);
	}
	return std::nullopt;
}

std::optional<Task> TaskScheduler::TryLocalPop() const {
	Task task;
	if (local_queue_->TryDequeue(task)) {
		XAMP_LOG_D(logger_, "Pop local queue ({}).", local_queue_->size());
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
		if (workstealing_queue_list_.at(index)->TryDequeue(task)) {
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
	threads_.emplace_back([i, this, priority]() mutable {
		// Avoid 64K Aliasing in L1 Cache (Intel hyper-threading)
		const auto L1_padding_buffer =
			MakeStackBuffer<uint8_t>((std::min)(kInitL1CacheLineSize * i,
			kMaxL1CacheLineSize));
		static thread_local PRNG prng;
		auto thread_id = GetCurrentThreadId();

		local_queue_ = workstealing_queue_list_[i].get();

		SetThreadPriority(priority);
		SetWorkerThreadName(i);

		XAMP_LOG_D(logger_, "Worker Thread {} ({}) start.", thread_id, i);
		std::chrono::milliseconds timeout(0);

		while (!is_stopped_) {
			auto task = TryLocalPop();           

			if (!task) {
				task = TryDequeueSharedQueue();
				if (!task) {
					auto steal_index = 0;
					do {
						steal_index = prng.NextSize(max_thread_ - 1);
					} while (steal_index != i);
					task = TrySteal(steal_index);
                    if (!task) {
                        /*if (local_queue_->size() > max_thread_) {
							timeout = std::chrono::milliseconds(0);
                        } else {
                            timeout = kDefaultTimeout;
                        }*/
                        task = TryDequeueSharedQueue(timeout);
						if (!task) {
							continue;
						}
					}
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
