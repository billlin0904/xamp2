#include <sstream>
#include <base/platform_thread.h>
#include <base/threadpool.h>

namespace xamp::base {

void SetCurrentThreadName(size_t index) {
	std::ostringstream ostr;
	ostr << "Work Thread(" << index << ")";
	SetThreadName(ostr.str());
}

namespace DefaultThreadPool {
ThreadPool& GetThreadPool() {
    static ThreadPool default_thread_pool;
    return default_thread_pool;
}
}

ThreadPool::ThreadPool()
    : scheduler_() {
}

}

