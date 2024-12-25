#include <benchmark/benchmark.h>

#include <base/trackinfo.h>
#include <base/threadpoolexecutor.h>
#include <base/spinlock.h>
#include <base/memory.h>
#include <base/rng.h>
#include <base/dataconverter.h>
#include <base/lrucache.h>
#include <base/stl.h>
#include <base/platform.h>
#include <base/uuid.h>
#include <base/shared_singleton.h>
#include <base/singleton.h>

#include <base/logger_impl.h>
#include <base/executor.h>
#include <base/uuidof.h>
#include <base/sfc64.h>
#include <base/mpmc_queue.h>

#include <stream/fft.h>
#include <stream/stream.h>
#include <stream/api.h>
#include <stream/ifileencoder.h>

#include <player/api.h>

#ifdef XAMP_OS_WIN
#include <base/simd.h>
#else
#include <uuid/uuid.h>
#endif

#include <iostream>
#include <numeric>
#include <vector>
#include <unordered_map>
#include <execution>
#include <unordered_set>

#include <base/dll.h>

using namespace xamp::player;
using namespace xamp::base;
using namespace xamp::stream;

XAMP_DECLARE_LOG_NAME(BM_ThreadPool);

bool IsPrime(size_t n) {
    if (n < 2) return false;
    auto N = static_cast<size_t>(std::sqrt(n));
    for (size_t i = 2; i <= N; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

constexpr size_t kThreadPoolTestTaskSize = 1000;

static void BM_ThreadPoolBaseLine(benchmark::State& state) {
    const size_t max_thread = state.range(0);
    std::vector<uint32_t> n(kThreadPoolTestTaskSize);
    std::generate(n.begin(), n.end(), []() {
        return Singleton<PRNG>::GetInstance().NextUInt32();
        });
	for (auto _ : state) {
		std::atomic<int64_t> prime_count{ 0 };
		for (auto item : n) {
			if (IsPrime(item)) {
				++prime_count;
			}
		}
		benchmark::DoNotOptimize(prime_count.load());
	}
}

static void BM_ThreadPool(benchmark::State& state) {
    const size_t max_thread = state.range(0);

    auto thread_pool = ThreadPoolBuilder::MakeThreadPool(
        XAMP_LOG_NAME(BM_ThreadPool),
        ThreadPriority::PRIORITY_NORMAL,
        max_thread);

    XampLoggerFactory.GetLogger(XAMP_LOG_NAME(BM_ThreadPool))
        ->SetLevel(LOG_LEVEL_OFF);

    std::vector<uint32_t> n(kThreadPoolTestTaskSize);
    PRNG prng;
    std::generate(n.begin(), n.end(), [&prng]() {
        return prng.NextUInt32();
        });
    std::atomic<int64_t> prime_count{0};
    std::stop_token stop_token;

    for (auto _ : state) {
        Executor::ParallelFor(thread_pool.get(), stop_token, n, [&prime_count](auto item) {
            if (IsPrime(item)) {
                ++prime_count;
            }
            });
        benchmark::DoNotOptimize(prime_count.load());
    }
}

auto std_parallel_for = [](auto& items, auto&& f) {
    auto begin = std::begin(items);
    size_t size = std::distance(begin, std::end(items));
    size_t batches = std::thread::hardware_concurrency() / 2 + 1;
    for (size_t i = 0; i < size; ++i) {
        std::vector<std::shared_future<void>> futures((std::min)(size - i, batches));
        for (auto& ff : futures) {
            ff = std::async(std::launch::async, [f, begin, i]() -> void {
                f(*(begin + i));
                });
            ++i;
        }
        for (auto& ff : futures) {
            ff.wait();
        }
    }
    };

static void BM_StdThreadPool(benchmark::State& state) {
    std::vector<uint32_t> n(kThreadPoolTestTaskSize);
    PRNG prng;
    std::generate(n.begin(), n.end(), [&prng]() {
        return prng.NextUInt32();
        });
    std::atomic<int64_t> prime_count;
    for (auto _ : state) {
        std_parallel_for(n, [&prime_count](auto item) {
            if (IsPrime(item)) {
                ++prime_count;
            }
            });
        benchmark::DoNotOptimize(prime_count.load());
    }
}

static void BM_ConvertInt8ToInt8SSE(benchmark::State& state) {
    const size_t frames = static_cast<size_t>(state.range(0));
    PRNG prng;
    auto input = prng.NextBytes(2 * frames);
    std::vector<int8_t> left(frames), right(frames);
    for (auto _ : state) {
        ConvertInt8ToInt8SSE(input.data(), left.data(), right.data(), frames);
        benchmark::DoNotOptimize(left);
        benchmark::DoNotOptimize(right);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * frames);
}

static void BM_ConvertFloatToInt32SSE(benchmark::State& state) {
    const size_t frames = static_cast<size_t>(state.range(0));
    PRNG prng;
    auto input = prng.NextSingles(2 * frames);
    std::vector<int32_t> left(frames), right(frames);
    for (auto _ : state) {
        ConvertFloatToInt32SSE(input.data(), left.data(), right.data(), frames);
        benchmark::DoNotOptimize(left);
        benchmark::DoNotOptimize(right);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * frames);
}

static void BM_ConvertFloatToFloatSSE(benchmark::State& state) {
    const size_t frames = static_cast<size_t>(state.range(0));
    PRNG prng;
    auto input = prng.NextSingles(2 * frames);
    std::vector<float> left(frames), right(frames);
    for (auto _ : state) {
        ConvertFloatToFloatSSE(input.data(), left.data(), right.data(), frames);
        benchmark::DoNotOptimize(left);
        benchmark::DoNotOptimize(right);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * frames);
}

static void BM_ConvertFloatToInt24SSE(benchmark::State& state) {
    const size_t frames = static_cast<size_t>(state.range(0));
    PRNG prng;
    auto input = prng.NextSingles(2 * frames);
    std::vector<int32_t> left(frames), right(frames);
	for (auto _ : state) {
        ConvertFloatToInt24SSE(input.data(), left.data(), right.data(), frames);
        benchmark::DoNotOptimize(left);
        benchmark::DoNotOptimize(right);
	}
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * frames);
}

static void BM_ConvertFloatToInt24(benchmark::State& state) {
    const size_t frames = static_cast<size_t>(state.range(0));
    PRNG prng;
    auto input = prng.NextSingles(2 * frames);
    std::vector<int32_t> left(frames), right(frames);
    for (auto _ : state) {
        ConvertFloatToInt24(input.data(), left.data(), right.data(), frames);
        benchmark::DoNotOptimize(left);
        benchmark::DoNotOptimize(right);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * frames);
}

BENCHMARK(BM_ConvertFloatToInt24)->RangeMultiplier(2)->Range(256, 256);

BENCHMARK(BM_ConvertInt8ToInt8SSE)->RangeMultiplier(2)->Range(256, 256);
BENCHMARK(BM_ConvertFloatToInt24SSE)->RangeMultiplier(2)->Range(256, 256);
BENCHMARK(BM_ConvertFloatToFloatSSE)->RangeMultiplier(2)->Range(256, 256);
BENCHMARK(BM_ConvertFloatToInt32SSE)->RangeMultiplier(2)->Range(256, 256);

BENCHMARK(BM_ThreadPoolBaseLine);
BENCHMARK(BM_ThreadPool)->Arg(2)->Arg(4)->Arg(8)->Arg(16)->Arg(32);
BENCHMARK(BM_StdThreadPool)->Arg(2)->Arg(4)->Arg(8)->Arg(16)->Arg(32);

int main(int argc, char** argv) {
    XampLoggerFactory
        .AddDebugOutput()
        .Startup();
    // For debug use!
    XAMP_LOG_DEBUG("Logger init success.");
    const auto components_path = GetComponentsFilePath();
    if (!AddSharedLibrarySearchDirectory(components_path)) {
        XAMP_LOG_ERROR("AddSharedLibrarySearchDirectory return fail! ({})", GetLastErrorMessage());
        return -1;
    }
    LoadBassLib();
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return -1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
}
