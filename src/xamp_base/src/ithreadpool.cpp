#include <base/logger.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/ithreadpoolexecutor.h>

XAMP_BASE_NAMESPACE_BEGIN
namespace {
	constexpr auto kMaxPlaybackThreadPoolSize{ 4 };
	constexpr auto kMaxOutputDeviceThreadPoolSize{ 2 };
	constexpr auto kMaxBackgroundThreadPoolSize{ 8 };

	XAMP_DECLARE_LOG_NAME(OutputDeviceThreadPool);
	XAMP_DECLARE_LOG_NAME(BackgroundThreadPool);
	XAMP_DECLARE_LOG_NAME(PlaybackThreadPool);

	CpuAffinity GetBackgroundCpuAffinity() {
		return CpuAffinity::kAll;
	}

	CpuAffinity GetOutputCpuAffinity() {
		return CpuAffinity(0, false);
	}
}

AlignPtr<IThreadPoolExecutor> ThreadPoolBuilder::MakeThreadPool(const std::string_view& pool_name,
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

AlignPtr<IThreadPoolExecutor> ThreadPoolBuilder::MakeThreadPool(const std::string_view& pool_name,
	TaskSchedulerPolicy policy,
	TaskStealPolicy steal_policy) {
	return MakeThreadPool(pool_name,
		ThreadPriority::PRIORITY_NORMAL,
		CpuAffinity::kAll,
		std::thread::hardware_concurrency(),
		policy,
		steal_policy);
}

AlignPtr<IThreadPoolExecutor> ThreadPoolBuilder::MakeThreadPool(
	const std::string_view& pool_name,
	ThreadPriority priority,
	TaskSchedulerPolicy policy,
	TaskStealPolicy steal_policy) {
	return MakeThreadPool(pool_name,
		priority,
		CpuAffinity::kAll,
		std::thread::hardware_concurrency(),
		policy,
		steal_policy);
}

AlignPtr<IThreadPoolExecutor> ThreadPoolBuilder::MakeOutputTheadPool() {
	return MakeAlign<IThreadPoolExecutor, ThreadPoolExecutor>(kOutputDeviceThreadPoolLoggerName,
		kMaxOutputDeviceThreadPoolSize,
		GetOutputCpuAffinity(),
		ThreadPriority::PRIORITY_HIGHEST);
}

AlignPtr<IThreadPoolExecutor> ThreadPoolBuilder::MakeBackgroundThreadPool() {
	return MakeAlign<IThreadPoolExecutor, ThreadPoolExecutor>(kBackgroundThreadPoolLoggerName,
		kMaxBackgroundThreadPoolSize,
		GetBackgroundCpuAffinity(),
		ThreadPriority::PRIORITY_BACKGROUND);
}

AlignPtr<IThreadPoolExecutor> ThreadPoolBuilder::MakePlaybackThreadPool() {
	return MakeAlign<IThreadPoolExecutor, ThreadPoolExecutor>(kPlaybackThreadPoolLoggerName,
		kMaxPlaybackThreadPoolSize,
		GetBackgroundCpuAffinity(),
		ThreadPriority::PRIORITY_NORMAL);
}

XAMP_BASE_NAMESPACE_END
