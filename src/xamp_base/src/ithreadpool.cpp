#include <base/logger.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/ithreadpoolexecutor.h>

XAMP_BASE_NAMESPACE_BEGIN

inline constexpr auto kMaxPlaybackThreadPoolSize{ 4 };
inline constexpr auto kMaxWASAPIThreadPoolSize{ 2 };

AlignPtr<IThreadPoolExecutor> MakeThreadPoolExecutor(const std::string_view& pool_name,
    ThreadPriority priority,
    CpuAffinity affinity,
    uint32_t max_thread,
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
        CpuAffinity::kAll,
        std::thread::hardware_concurrency(),
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
        CpuAffinity::kAll,
        std::thread::hardware_concurrency(),
        policy,
        steal_policy);
}

XAMP_DECLARE_LOG_NAME(PlaybackThreadPool);
XAMP_DECLARE_LOG_NAME(WASAPIThreadPool);

IThreadPoolExecutor& GetPlaybackThreadPool() {
    static ThreadPoolExecutor executor(kPlaybackThreadPoolLoggerName,
        kMaxPlaybackThreadPoolSize,
        CpuAffinity::kAll);
	return executor;
}

IThreadPoolExecutor& GetWasapiThreadPool() {
    static const CpuAffinity wasapi_cpu_aff(1, false);
    static ThreadPoolExecutor executor(kWASAPIThreadPoolLoggerName,
        kMaxWASAPIThreadPoolSize,
        wasapi_cpu_aff,
        ThreadPriority::HIGHEST);
    return executor;
}

XAMP_BASE_NAMESPACE_END
