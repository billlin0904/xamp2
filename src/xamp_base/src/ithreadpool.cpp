#include <base/logger.h>
#include <base/threadpool.h>
#include <base/ithreadpool.h>

namespace xamp::base {

inline constexpr auto kMaxStreamReaderThreadPoolSize{ 4 };
inline constexpr auto kMaxWASAPIThreadPoolSize{ 4 };

IThreadPool& StreamReaderThreadPool() {
	static ThreadPool threadpool(kStreamThreadPoolLoggerName, kMaxStreamReaderThreadPoolSize);
	return threadpool;
}

IThreadPool& WASAPIThreadPool() {
	static ThreadPool wasapi_threadpool(kWASAPIThreadPoolLoggerName, kMaxWASAPIThreadPoolSize, 0);
	return wasapi_threadpool;
}
}
