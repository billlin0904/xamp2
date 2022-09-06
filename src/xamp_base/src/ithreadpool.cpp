#include <base/logger.h>
#include <base/threadpool.h>
#include <base/logger_impl.h>
#include <base/ithreadpool.h>

namespace xamp::base {

inline constexpr auto kMaxPlaybackThreadPoolSize{ 4 };

AlignPtr<IThreadPool> MakeThreadPool(const std::string_view& pool_name,
    ThreadPriority priority,
    uint32_t max_thread,
    int32_t affinity,    
    TaskSchedulerPolicy policy,
    TaskStealPolicy steal_policy) {
    return MakeAlign<IThreadPool, ThreadPool>(pool_name,
        policy,
        steal_policy,
        max_thread,
        affinity,
        priority);
}

IThreadPool& GetPlaybackThreadPool() {
    static ThreadPool threadpool(kPlaybackThreadPoolLoggerName,
        kMaxPlaybackThreadPoolSize,
        kDefaultAffinityCpuCore);
	return threadpool;
}

}
