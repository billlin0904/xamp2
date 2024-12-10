#include <base/logger.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/ithreadpoolexecutor.h>

XAMP_BASE_NAMESPACE_BEGIN
namespace {
	constexpr auto kMaxPlaybackThreadPoolSize{ 4 };
	constexpr auto kMaxBackgroundThreadPoolSize{ 8 };

	XAMP_DECLARE_LOG_NAME(BackgroundThreadPool);
	XAMP_DECLARE_LOG_NAME(PlaybackThreadPool);
}

std::shared_ptr<IThreadPoolExecutor> ThreadPoolBuilder::MakeThreadPool(const std::string_view& pool_name,
	ThreadPriority priority,
	uint32_t max_thread) {
	return MakeShared<IThreadPoolExecutor, ThreadPoolExecutor>(pool_name,
		max_thread,
		priority);
}

std::shared_ptr<IThreadPoolExecutor> ThreadPoolBuilder::MakeBackgroundThreadPool() {
	return MakeShared<IThreadPoolExecutor, ThreadPoolExecutor>(kBackgroundThreadPoolLoggerName,
		kMaxBackgroundThreadPoolSize,
		ThreadPriority::PRIORITY_BACKGROUND);
}

std::shared_ptr<IThreadPoolExecutor> ThreadPoolBuilder::MakePlaybackThreadPool() {
	return MakeShared<IThreadPoolExecutor, ThreadPoolExecutor>(kPlaybackThreadPoolLoggerName,
		kMaxPlaybackThreadPoolSize,
		ThreadPriority::PRIORITY_NORMAL);
}

XAMP_BASE_NAMESPACE_END
