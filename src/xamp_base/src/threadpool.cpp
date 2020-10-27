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
	scheduler_.Destroy();
}

void ThreadPool::SetAffinityMask(int32_t core) {
	scheduler_.SetAffinityMask(core);
}

ThreadPool& ThreadPool::Default() {
	static ThreadPool default_pool;
	return default_pool;
}

}
