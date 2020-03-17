#include <sstream>
#include <base/platform_thread.h>
#include <base/threadpool.h>

namespace xamp::base {

void SetCurrentThreadName(size_t index) {
	std::ostringstream ostr;
	ostr << "Work Thread(" << index << ")";
	SetThreadName(ostr.str());
}

ThreadPool::ThreadPool()
    : scheduler_() {
}

}

