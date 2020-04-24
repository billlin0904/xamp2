#include <sstream>
#include <algorithm>

#include <base/platform_thread.h>
#include <base/threadpool.h>

namespace xamp::base {

ThreadPool::ThreadPool()
    : scheduler_((std::min)(std::thread::hardware_concurrency(), MAX_THREAD)) {
}

size_t ThreadPool::GetActiveThreadCount() const {
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

