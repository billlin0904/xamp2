#include <base/threadpoolbuilder.h>

#include <base/logger.h>
#include <base/memory.h>
#include <base/threadpool.h>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
    constexpr auto kMaxPlaybackThreadPoolSize{ 4 };
    constexpr auto kMaxPlayerThreadPoolSize{ 4 };
    constexpr auto kMaxBackgroundThreadPoolSize{ 12 };

    XAMP_DECLARE_LOG_NAME(BackgroundThreadPool);
    XAMP_DECLARE_LOG_NAME(PlaybackThreadPool);
    XAMP_DECLARE_LOG_NAME(PlayerThreadPool);
}

std::shared_ptr<IThreadPool> ThreadPoolBuilder::makeThreadPool(const std::string_view& pool_name,
    uint32_t max_thread,
    size_t bulk_size,
    ThreadPriority priority) {
    return makeShared<IThreadPool, ThreadPool>(pool_name,
        max_thread,
        bulk_size,
        priority);
}

std::shared_ptr<IThreadPool> ThreadPoolBuilder::makeBackgroundThreadPool() {
    return makeThreadPool(XAMP_LOG_NAME(BackgroundThreadPool),
        kMaxBackgroundThreadPoolSize,
        kMaxBackgroundThreadPoolSize / 2,
        ThreadPriority::PRIORITY_BACKGROUND);
}

std::shared_ptr<IThreadPool> ThreadPoolBuilder::makePlaybackThreadPool() {
    return makeThreadPool(XAMP_LOG_NAME(PlaybackThreadPool),
        kMaxPlaybackThreadPoolSize,
        1,
        ThreadPriority::PRIORITY_HIGHEST);
}

std::shared_ptr<IThreadPool> ThreadPoolBuilder::makePlayerThreadPool() {
    return makeThreadPool(XAMP_LOG_NAME(PlayerThreadPool),
        kMaxPlayerThreadPoolSize,
        1,
        ThreadPriority::PRIORITY_NORMAL);
}

XAMP_BASE_NAMESPACE_END
