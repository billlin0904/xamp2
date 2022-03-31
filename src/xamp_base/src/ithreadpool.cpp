#include <base/logger.h>
#include <base/threadpool.h>
#include <base/itaskschedulerpolicy.h>
#include <base/ithreadpool.h>

namespace xamp::base {

inline constexpr auto kMaxPlaybackThreadPoolSize{ 2 };
inline constexpr auto kMaxWASAPIThreadPoolSize{ 2 };

uint32_t GetIdealThreadPoolSize(double cpu_utilization, 
    std::chrono::milliseconds wait_time,
    std::chrono::milliseconds service_time) {
	const auto ratio = static_cast<double>(wait_time.count()) / static_cast<double>(service_time.count());
    return static_cast<uint32_t>(std::thread::hardware_concurrency() * cpu_utilization * (1.0 + ratio));
}

AlignPtr<IThreadPool> MakeThreadPool(const std::string_view& pool_name,
    TaskSchedulerPolicy policy,
    ThreadPriority priority,
    uint32_t max_thread,
    int32_t affinity) {
    return MakeAlign<IThreadPool, ThreadPool>(pool_name,
        policy,
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
