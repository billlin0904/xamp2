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

ScopedPtr<IThreadPoolExecutor> ThreadPoolBuilder::MakeThreadPool(const std::string_view& pool_name,
	ThreadPriority priority,
	CpuAffinity affinity,
	uint32_t max_thread) {
	return MakeAlign<IThreadPoolExecutor, ThreadPoolExecutor>(pool_name,
		max_thread,
		affinity,
		priority);
}

ScopedPtr<IThreadPoolExecutor> ThreadPoolBuilder::MakeOutputTheadPool() {
	return MakeAlign<IThreadPoolExecutor, ThreadPoolExecutor>(kOutputDeviceThreadPoolLoggerName,
		kMaxOutputDeviceThreadPoolSize,
		GetOutputCpuAffinity(),
		ThreadPriority::PRIORITY_HIGHEST);
}

ScopedPtr<IThreadPoolExecutor> ThreadPoolBuilder::MakeBackgroundThreadPool() {
	return MakeAlign<IThreadPoolExecutor, ThreadPoolExecutor>(kBackgroundThreadPoolLoggerName,
		kMaxBackgroundThreadPoolSize,
		GetBackgroundCpuAffinity(),
		ThreadPriority::PRIORITY_BACKGROUND);
}

ScopedPtr<IThreadPoolExecutor> ThreadPoolBuilder::MakePlaybackThreadPool() {
	return MakeAlign<IThreadPoolExecutor, ThreadPoolExecutor>(kPlaybackThreadPoolLoggerName,
		kMaxPlaybackThreadPoolSize,
		GetBackgroundCpuAffinity(),
		ThreadPriority::PRIORITY_NORMAL);
}

XAMP_BASE_NAMESPACE_END
