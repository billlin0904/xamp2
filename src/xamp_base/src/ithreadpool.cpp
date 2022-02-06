#include <base/logger.h>
#include <base/threadpool.h>
#include <base/ithreadpool.h>

namespace xamp::base {

inline constexpr auto kMaxPlaybackThreadPoolSize{ 4 };
inline constexpr auto kMaxWASAPIThreadPoolSize{ 2 };
inline constexpr auto kMaxFileThreadPoolSize{ 8 };

AlignPtr<IThreadPool> MakeThreadPool(const std::string_view& pool_name) {
    return MakeAlign<IThreadPool, ThreadPool>(pool_name);
}

IThreadPool& PlaybackThreadPool() {
    static ThreadPool threadpool(kPlaybackThreadPoolLoggerName, kMaxPlaybackThreadPoolSize);
	return threadpool;
}

IThreadPool& WASAPIThreadPool() {
    static ThreadPool wasapi_threadpool(kWASAPIThreadPoolLoggerName, kMaxWASAPIThreadPoolSize, 0);
	return wasapi_threadpool;
}

}
