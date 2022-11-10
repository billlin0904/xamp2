#include <base/logger.h>
#include <base/threadpool.h>
#include <base/logger_impl.h>
#include <base/ithreadpool.h>

namespace xamp::base {

inline constexpr auto kMaxPlaybackThreadPoolSize{ 4 };
inline constexpr auto kMaxWASAPIThreadPoolSize{ 2 };

AlignPtr<IThreadPoolExcutor> MakeThreadPoolExcutor(const std::string_view& pool_name,
    ThreadPriority priority,
    uint32_t max_thread,
    CpuAffinity affinity,
    TaskSchedulerPolicy policy,
    TaskStealPolicy steal_policy) {
    return MakeAlign<IThreadPoolExcutor, ThreadPoolExcutor>(pool_name,
        policy,
        steal_policy,
        max_thread,
        affinity,
        priority);
}

AlignPtr<IThreadPoolExcutor> MakeThreadPoolExcutor(const std::string_view& pool_name,
    TaskSchedulerPolicy policy,
    TaskStealPolicy steal_policy) {
    return MakeThreadPoolExcutor(pool_name,
        ThreadPriority::NORMAL,
        std::thread::hardware_concurrency(),
        kDefaultAffinityCpuCore,
        policy,
        steal_policy);
}

IThreadPoolExcutor& GetPlaybackThreadPool() {
    static ThreadPoolExcutor threadpool(kPlaybackThreadPoolLoggerName,
        kMaxPlaybackThreadPoolSize,
        kDefaultAffinityCpuCore);
	return threadpool;
}

IThreadPoolExcutor& GetWASAPIThreadPool() {
    static const CpuAffinity wasapi_cpu_aff{ 1 };
    static ThreadPoolExcutor threadpool(kWASAPIThreadPoolLoggerName,
        kMaxWASAPIThreadPoolSize,
        wasapi_cpu_aff,
        ThreadPriority::HIGHEST);
    return threadpool;
}

}
