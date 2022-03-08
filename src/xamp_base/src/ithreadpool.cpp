#include <base/logger.h>
#include <base/threadpool.h>
#include <base/ithreadpool.h>

namespace xamp::base {

inline constexpr auto kMaxPlaybackThreadPoolSize{ 2 };
inline constexpr auto kMaxWASAPIThreadPoolSize{ 2 };

AlignPtr<IThreadPool> MakeThreadPool(const std::string_view& pool_name, int32_t max_thread, int32_t affinity, ThreadPriority priority) {
    return MakeAlign<IThreadPool, ThreadPool>(pool_name, 
        max_thread,
        affinity,
        priority);
}

IThreadPool& PlaybackThreadPool() {
    static ThreadPool threadpool(kPlaybackThreadPoolLoggerName,
        kMaxPlaybackThreadPoolSize,
        kDefaultAffinityCpuCore);
	return threadpool;
}

IThreadPool& WASAPIThreadPool() {
    static ThreadPool wasapi_threadpool(kWASAPIThreadPoolLoggerName,
        kMaxWASAPIThreadPoolSize,
        0);
	return wasapi_threadpool;
}

}
