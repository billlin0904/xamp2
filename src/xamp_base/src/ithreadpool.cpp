#include <base/logger.h>
#include <base/threadpool.h>
#ifdef XAMP_OS_WIN
#include <base/win32/win32_threadpool.h>
#endif
#include <base/ithreadpool.h>

namespace xamp::base {

inline constexpr auto kMaxPlaybackThreadPoolSize{ 4 };
inline constexpr auto kMaxWASAPIThreadPoolSize{ 4 };

#ifdef XAMP_OS_WIN
//using FastThreadPool = win32::ThreadPool;
using FastThreadPool = ThreadPool;
#else
using FastThreadPool = ThreadPool;
#endif

AlignPtr<IThreadPool> MakeThreadPool(const std::string_view& pool_name) {
    return MakeAlign<IThreadPool, FastThreadPool>(pool_name);
}

IThreadPool& PlaybackThreadPool() {
    static FastThreadPool threadpool(kPlaybackThreadPoolLoggerName, kMaxPlaybackThreadPoolSize);
	return threadpool;
}

IThreadPool& WASAPIThreadPool() {
	static FastThreadPool wasapi_threadpool(kWASAPIThreadPoolLoggerName, kMaxWASAPIThreadPoolSize, 0);
	return wasapi_threadpool;
}
}
