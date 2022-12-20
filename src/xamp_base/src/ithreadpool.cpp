#include <base/logger.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/ithreadpoolexecutor.h>

namespace xamp::base {

inline constexpr auto kMaxPlaybackThreadPoolSize{ 4 };
inline constexpr auto kMaxWASAPIThreadPoolSize{ 2 };

AlignPtr<IThreadPoolExecutor> MakeThreadPoolExecutor(const std::string_view& pool_name,
    ThreadPriority priority,
    uint32_t max_thread,
    CpuAffinity affinity,
    TaskSchedulerPolicy policy,
    TaskStealPolicy steal_policy) {
    return MakeAlign<IThreadPoolExecutor, ThreadPoolExecutor>(pool_name,
        policy,
        steal_policy,
        max_thread,
        affinity,
        priority);
}

AlignPtr<IThreadPoolExecutor> MakeThreadPoolExecutor(const std::string_view& pool_name,
    TaskSchedulerPolicy policy,
    TaskStealPolicy steal_policy) {
    return MakeThreadPoolExecutor(pool_name,
        ThreadPriority::NORMAL,
        std::thread::hardware_concurrency(),
        kDefaultAffinityCpuCore,
        policy,
        steal_policy);
}

AlignPtr<IThreadPoolExecutor> MakeThreadPoolExecutor(
    const std::string_view& pool_name,
    ThreadPriority priority,
    TaskSchedulerPolicy policy,
    TaskStealPolicy steal_policy) {
    return MakeThreadPoolExecutor(pool_name,
        priority,
        std::thread::hardware_concurrency(),
        kDefaultAffinityCpuCore,
        policy,
        steal_policy);
}

XAMP_DECLARE_LOG_NAME(PlaybackThreadPool);
XAMP_DECLARE_LOG_NAME(WASAPIThreadPool);

IThreadPoolExecutor& GetPlaybackThreadPool() {
    static ThreadPoolExecutor executor(kPlaybackThreadPoolLoggerName,
        kMaxPlaybackThreadPoolSize,
        kDefaultAffinityCpuCore);
	return executor;
}

IThreadPoolExecutor& GetWASAPIThreadPool() {
    static const CpuAffinity wasapi_cpu_aff{ 1 };
    static ThreadPoolExecutor executor(kWASAPIThreadPoolLoggerName,
        kMaxWASAPIThreadPoolSize,
        wasapi_cpu_aff,
        ThreadPriority::HIGHEST);
    return executor;
}

}
