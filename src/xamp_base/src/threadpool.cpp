#include <algorithm>
#include <base/threadpool.h>

namespace xamp::base {

ThreadPool::ThreadPool()
    : scheduler_((std::min)(std::thread::hardware_concurrency(), kMaxThread)) {
}

size_t ThreadPool::GetActiveThreadCount() const noexcept {
	return scheduler_.GetActiveThreadCount();
}

void ThreadPool::Stop() {
	scheduler_.Destory();
}

ThreadPool& ThreadPool::DefaultThreadPool() {
	static ThreadPool default_thread_pool;
	return default_thread_pool;
}

}
