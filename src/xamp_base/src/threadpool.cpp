#if 0
#include <base/platform_thread.h>
#include <base/threadpool.h>

namespace xamp::base {

void SetCurrentThreadName(int32_t index) {
	std::ostringstream ostr;
	ostr << "Work Thread(" << index << ")";
	PlatformThread::SetThreadName(ostr.str());
}

ThreadPool::ThreadPool()
    : scheduler_() {
}

}
#endif
