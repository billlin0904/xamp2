#include <base/logger.h>
#include <base/threadpool.h>
#include <base/logger_impl.h>
#include <base/ithreadpool.h>

namespace xamp::base {

inline constexpr auto kMaxPlaybackThreadPoolSize{ 4 };
inline constexpr auto kMaxWASAPIThreadPoolSize{ 2 };
inline constexpr auto kMaxDSPThreadPoolSize{ 4 };

AlignPtr<IThreadPool> MakeThreadPool(const std::string_view& pool_name,
    TaskSchedulerPolicy policy,
    TaskStealPolicy steal_policy,
    ThreadPriority priority,
    uint32_t max_thread,
    int32_t affinity) {
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

IThreadPool& GetWASAPIThreadPool() {
    static ThreadPool threadpool(kWASAPIThreadPoolLoggerName,
        kMaxWASAPIThreadPoolSize,
        0);
	return threadpool;
}

IThreadPool& GetDSPThreadPool() {
    static ThreadPool threadpool(kDSPThreadPoolLoggerName,
        kMaxDSPThreadPoolSize,
        kDefaultAffinityCpuCore);
    return threadpool;
}

}
