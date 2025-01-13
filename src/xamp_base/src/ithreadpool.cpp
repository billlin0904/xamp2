#include <base/logger.h>
#include <base/threadpoolexecutor.h>
#include <base/logger_impl.h>
#include <base/ithreadpoolexecutor.h>

XAMP_BASE_NAMESPACE_BEGIN
namespace {
	constexpr auto kMaxPlaybackThreadPoolSize{ 4 };
	constexpr auto kMaxPlayerThreadPoolSize{ 4 };
	constexpr auto kMaxBackgroundThreadPoolSize{ 12 };

	XAMP_DECLARE_LOG_NAME(BackgroundThreadPool);
	XAMP_DECLARE_LOG_NAME(PlaybackThreadPool);
	XAMP_DECLARE_LOG_NAME(PlayerThreadPool);
}

std::shared_ptr<IThreadPoolExecutor> ThreadPoolBuilder::MakeThreadPool(const std::string_view& pool_name,
	uint32_t max_thread,
	size_t bulk_size,
	ThreadPriority priority) {
	return MakeShared<IThreadPoolExecutor, ThreadPoolExecutor>(pool_name,
		max_thread,
		bulk_size,
		priority);
}

std::shared_ptr<IThreadPoolExecutor> ThreadPoolBuilder::MakeBackgroundThreadPool() {
	return MakeThreadPool(XAMP_LOG_NAME(BackgroundThreadPool),
		kMaxBackgroundThreadPoolSize,
		kMaxBackgroundThreadPoolSize / 2,
		ThreadPriority::PRIORITY_BACKGROUND);
}

std::shared_ptr<IThreadPoolExecutor> ThreadPoolBuilder::MakePlaybackThreadPool() {
	return MakeThreadPool(XAMP_LOG_NAME(PlaybackThreadPool),
		kMaxPlaybackThreadPoolSize,
		1,
		ThreadPriority::PRIORITY_NORMAL);
}

std::shared_ptr<IThreadPoolExecutor> ThreadPoolBuilder::MakePlayerThreadPool() {
	return MakeThreadPool(XAMP_LOG_NAME(PlayerThreadPool),
		kMaxPlayerThreadPoolSize,
		1,
		ThreadPriority::PRIORITY_NORMAL);
}

XAMP_BASE_NAMESPACE_END
