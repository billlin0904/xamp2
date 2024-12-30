#include <base/logger.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/ithreadpoolexecutor.h>

XAMP_BASE_NAMESPACE_BEGIN
namespace {
	constexpr auto kMaxPlaybackThreadPoolSize{ 8 };
	constexpr auto kMaxPlayerThreadPoolSize{ 2 };
	constexpr auto kMaxBackgroundThreadPoolSize{ 12 };

	XAMP_DECLARE_LOG_NAME(BackgroundThreadPool);
	XAMP_DECLARE_LOG_NAME(PlaybackThreadPool);
	XAMP_DECLARE_LOG_NAME(PlayerThreadPool);
}

std::shared_ptr<IThreadPoolExecutor> ThreadPoolBuilder::MakeThreadPool(const std::string_view& pool_name,
	uint32_t max_thread,
	ThreadPriority priority) {
	return MakeShared<IThreadPoolExecutor, ThreadPoolExecutor>(pool_name,
		max_thread,
		priority);
}

std::shared_ptr<IThreadPoolExecutor> ThreadPoolBuilder::MakeBackgroundThreadPool() {
	return MakeThreadPool(kBackgroundThreadPoolLoggerName,
		kMaxBackgroundThreadPoolSize,
		ThreadPriority::PRIORITY_BACKGROUND);
}

std::shared_ptr<IThreadPoolExecutor> ThreadPoolBuilder::MakePlaybackThreadPool() {
	return MakeThreadPool(kPlaybackThreadPoolLoggerName,
		kMaxPlaybackThreadPoolSize,
		ThreadPriority::PRIORITY_NORMAL);
}

std::shared_ptr<IThreadPoolExecutor> ThreadPoolBuilder::MakePlayerThreadPool() {
	return MakeThreadPool(kPlayerThreadPoolLoggerName,
		kMaxPlayerThreadPoolSize,
		ThreadPriority::PRIORITY_NORMAL);
}

XAMP_BASE_NAMESPACE_END
