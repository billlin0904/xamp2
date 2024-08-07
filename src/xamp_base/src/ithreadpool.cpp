#include <base/logger.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/ithreadpoolexecutor.h>

XAMP_BASE_NAMESPACE_BEGIN
namespace {
	constexpr auto kMaxPlaybackThreadPoolSize{4};
	constexpr auto kMaxOutputDeviceThreadPoolSize{2};
	constexpr auto kMaxBackgroundThreadPoolSize{8};

	CpuAffinity GetBackgroundCpuAffinity() {
		/*CpuAffinity affinity(-1, false);
		affinity.SetCpu(1);
		affinity.SetCpu(2);
		affinity.SetCpu(3);
		affinity.SetCpu(4);
		return affinity;*/
		return CpuAffinity::kAll;
	}
}

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
	                              ThreadPriority::PRIORITY_NORMAL,
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

XAMP_DECLARE_LOG_NAME(OutputDeviceThreadPool);
XAMP_DECLARE_LOG_NAME(BackgroundThreadPool);
XAMP_DECLARE_LOG_NAME(PlaybackThreadPool);

IThreadPoolExecutor& GetOutputDeviceThreadPool() {
	static const CpuAffinity cpu_aff(0, false);
	static ThreadPoolExecutor executor(kOutputDeviceThreadPoolLoggerName,
	                                   kMaxOutputDeviceThreadPoolSize,
	                                   cpu_aff,
	                                   ThreadPriority::PRIORITY_HIGHEST);
	return executor;
}

IThreadPoolExecutor& GetBackgroundThreadPool() {
	static ThreadPoolExecutor executor(kBackgroundThreadPoolLoggerName,
	                                   kMaxBackgroundThreadPoolSize,
	                                   GetBackgroundCpuAffinity(),
	                                   ThreadPriority::PRIORITY_BACKGROUND);
	return executor;
}

IThreadPoolExecutor& GetPlaybackThreadPool() {
	static ThreadPoolExecutor executor(kPlaybackThreadPoolLoggerName,
	                                   kMaxPlaybackThreadPoolSize,
	                                   GetBackgroundCpuAffinity(),
	                                   ThreadPriority::PRIORITY_BACKGROUND);
	return executor;
}

XAMP_BASE_NAMESPACE_END
