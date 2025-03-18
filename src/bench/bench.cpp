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
        max_thread,
        max_thread / 2,
        ThreadPriority::PRIORITY_NORMAL);

#ifndef _DEBUG
    XampLoggerFactory.GetLogger(XAMP_LOG_NAME(BM_ThreadPool))
        ->SetLevel(LOG_LEVEL_OFF);
#else
    XampLoggerFactory.GetLogger(XAMP_LOG_NAME(BM_ThreadPool))
        ->SetLevel(LOG_LEVEL_TRACE);
#endif
    std::vector<uint32_t> n(kThreadPoolTestTaskSize);
    PRNG prng;
    std::generate(n.begin(), n.end(), [&prng]() {
        return prng.NextUInt32();
        });
    std::atomic<int64_t> prime_count{0};

    auto task_lambda = [&prime_count, thread_pool](auto item) {
        if (IsPrime(item)) {
            ++prime_count;
        }
        thread_pool->Spawn([](auto stop_token) {
            });
    };

    for (auto _ : state) {
        Executor::ParallelFor(thread_pool.get(), n, task_lambda);
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

static void BM_FastFile_Write(benchmark::State& state) {
    const uint32_t bytes_to_write = static_cast<uint32_t>(state.range(0));
    const Path kTestFileName = std::filesystem::temp_directory_path() / "fastfile_write_test.bin";

    std::vector<char> buffer(bytes_to_write, 'x');
	for (auto _ : state) {
        FastFile file(kTestFileName, FastFileOpenMode::FAST_IO_WRITE);
		uint32_t bytes_written = 0;
		file.Write(buffer.data(), bytes_to_write, bytes_written);
        file.Close();
		benchmark::DoNotOptimize(bytes_written);
	}

    std::filesystem::remove(kTestFileName);
}

static void BM_STDFile_Write(benchmark::State& state) {
    const uint32_t bytes_to_write = static_cast<uint32_t>(state.range(0));
    const Path kTestFileName = std::filesystem::temp_directory_path() / "stdfile_write_test.bin";

    std::vector<char> buffer(bytes_to_write, 'x');
    for (auto _ : state) {
        std::ofstream ofs(kTestFileName, std::ios::binary | std::ios::trunc);
        ofs.write(buffer.data(), static_cast<std::streamsize>(bytes_to_write));
        ofs.close();
    }

    std::filesystem::remove(kTestFileName);
}

static void BM_FastFile_Read(benchmark::State& state) {
    const uint32_t bytes_to_read = static_cast<uint32_t>(state.range(0));
    const Path kTestFileName = std::filesystem::temp_directory_path() / "fastfile_read_test.bin";

    std::vector<char> buffer(bytes_to_read, 'x');
    std::vector<char> read_buffer(bytes_to_read, '\0');

    FastFile test_file(kTestFileName, FastFileOpenMode::FAST_IO_WRITE);
    uint32_t bytes_written = 0;
    test_file.Write(buffer.data(), bytes_to_read, bytes_written);
    test_file.Close();

    for (auto _ : state) {
        FastFile file(kTestFileName, FastFileOpenMode::FAST_IO_READ);
        uint32_t bytes_read = 0;
        file.Read(read_buffer.data(), bytes_to_read, bytes_read);
        file.Close();
        benchmark::DoNotOptimize(read_buffer);
        benchmark::DoNotOptimize(bytes_written);
    }

    std::filesystem::remove(kTestFileName);
}

static void BM_STDFile_Read(benchmark::State& state) {
    const uint32_t bytes_to_read = static_cast<uint32_t>(state.range(0));
    const Path kTestFileName = std::filesystem::temp_directory_path() / "stdfile_read_test.bin";

    std::vector<char> buffer(bytes_to_read, 'x');
    std::vector<char> read_buffer(bytes_to_read, '\0');

    std::ofstream ofs(kTestFileName, std::ios::binary | std::ios::trunc);
    ofs.write(buffer.data(), bytes_to_read);
    ofs.close();

    for (auto _ : state) {
        std::ifstream ifs(kTestFileName, std::ios::binary);
        ifs.read(read_buffer.data(), bytes_to_read);
        ifs.close();
        benchmark::DoNotOptimize(read_buffer);
    }
}

static void BM_FastFileWriteThreadPool(benchmark::State& state) {
    const size_t max_thread = state.range(0);

    auto thread_pool = ThreadPoolBuilder::MakeThreadPool(
        XAMP_LOG_NAME(BM_ThreadPool),
        max_thread,
        max_thread / 2,
        ThreadPriority::PRIORITY_NORMAL);

    XampLoggerFactory.GetLogger(XAMP_LOG_NAME(BM_ThreadPool))
        ->SetLevel(LOG_LEVEL_OFF);

    std::vector<uint32_t> n(kThreadPoolTestTaskSize);
    PRNG prng;
    std::generate(n.begin(), n.end(), [&prng]() {
        return prng.NextUInt32();
        });

    for (auto _ : state) {
        Executor::ParallelFor(thread_pool.get(), n, [&n](auto item) {
			auto file_name = GetSequentialUUID() + ".bin";
            const Path kTestFileName = std::filesystem::temp_directory_path() / file_name;
            FastFile test_file(kTestFileName, FastFileOpenMode::FAST_IO_WRITE);
            uint32_t bytes_written = 0;
            test_file.Write(n.data(), n.size() * sizeof(uint32_t), bytes_written);
            test_file.Close();
            std::filesystem::remove(kTestFileName);
            });
    }
}

static void BM_STDFileWriteThreadPool(benchmark::State& state) {
    const size_t max_thread = state.range(0);

    auto thread_pool = ThreadPoolBuilder::MakeThreadPool(
        XAMP_LOG_NAME(BM_ThreadPool),
        max_thread,
		max_thread / 2,
        ThreadPriority::PRIORITY_NORMAL);

    XampLoggerFactory.GetLogger(XAMP_LOG_NAME(BM_ThreadPool))
        ->SetLevel(LOG_LEVEL_OFF);

    std::vector<uint32_t> n(kThreadPoolTestTaskSize);
    PRNG prng;
    std::generate(n.begin(), n.end(), [&prng]() {
        return prng.NextUInt32();
        });

    for (auto _ : state) {
        Executor::ParallelFor(thread_pool.get(), n, [&n](auto item) {
            auto file_name = GetSequentialUUID() + ".bin";
            const Path kTestFileName = std::filesystem::temp_directory_path() / file_name;
            std::ofstream ofs(kTestFileName, std::ios::binary | std::ios::trunc);
            ofs.write(reinterpret_cast<const char*>(n.data()), static_cast<std::streamsize>(n.size() * sizeof(uint32_t)));
            ofs.close();
            std::filesystem::remove(kTestFileName);
            });
    }
}

static std::vector<int> GenerateRandomKeys(size_t n, unsigned seed = 1234) {
    std::vector<int> keys(n);
    PRNG prng;
    std::generate(keys.begin(), keys.end(), [&prng]() {
        return prng.NextInt32();
        });
    return keys;
}

// -----------------------------------------------------------------------------
// 基準測試部分
// -----------------------------------------------------------------------------
static void BM_V1_CreateAndDestroy(benchmark::State& state) {
    for (auto _ : state) {
        // 建立並銷毀, 不呼叫
        MoveOnlyFunctionVTB fn([](const std::stop_token&) {});
        benchmark::DoNotOptimize(fn);
    }
}

static void BM_V2_CreateAndDestroy(benchmark::State& state) {
    for (auto _ : state) {
        // 建立並銷毀, 不呼叫
        MoveOnlyFunctionSFO fn([](const std::stop_token&) {});
        benchmark::DoNotOptimize(fn);
    }
}

static void BM_V1_CreateAndInvoke(benchmark::State& state) {
    for (auto _ : state) {
        MoveOnlyFunctionVTB fn([](const std::stop_token&) {
            // 做點小事以免被編譯器最佳化
            int x = 1;
            benchmark::DoNotOptimize(x);
            });
        fn(std::stop_token{}); // 只呼叫一次
    }
}

static void BM_V2_CreateAndInvoke(benchmark::State& state) {
    for (auto _ : state) {
        MoveOnlyFunctionSFO fn([](const std::stop_token&) {
            int x = 1;
            benchmark::DoNotOptimize(x);
            });
        fn(std::stop_token{});
    }
}

// 測試 move 的效能: 兩個同類型物件間互相移動
static void BM_V1_MoveOperation(benchmark::State& state) {
    // 預先準備一個小型Lambda
    auto makeLambda = [] {
        return MoveOnlyFunctionVTB([](const std::stop_token&) {
            int x = 42;
            benchmark::DoNotOptimize(x);
            });
        };

    for (auto _ : state) {
        MoveOnlyFunctionVTB fn1 = makeLambda();
        MoveOnlyFunctionVTB fn2 = makeLambda();
        // 互相移動
        fn1 = std::move(fn2);
        benchmark::DoNotOptimize(fn1);
    }
}

static void BM_V2_MoveOperation(benchmark::State& state) {
    auto makeLambda = [] {
        return MoveOnlyFunctionSFO([](const std::stop_token&) {
            int x = 42;
            benchmark::DoNotOptimize(x);
            });
        };

    for (auto _ : state) {
        MoveOnlyFunctionSFO fn1 = makeLambda();
        MoveOnlyFunctionSFO fn2 = makeLambda();
        fn1 = std::move(fn2);
        benchmark::DoNotOptimize(fn1);
    }
}

// -----------------------------------------------------------------------------
// 註冊基準測試
// -----------------------------------------------------------------------------
BENCHMARK(BM_V1_CreateAndDestroy);
BENCHMARK(BM_V2_CreateAndDestroy);

BENCHMARK(BM_V1_CreateAndInvoke);
BENCHMARK(BM_V2_CreateAndInvoke);

BENCHMARK(BM_V1_MoveOperation);
BENCHMARK(BM_V2_MoveOperation);

// ============================================================
//  Part B.  std::unordered_map 基準測試
// ============================================================

// (1) 插入測試: Insert
static void BM_UnorderedMap_Insert(benchmark::State& state) {
    size_t n = static_cast<size_t>(state.range(0));
    auto keys = GenerateRandomKeys(n);

    for (auto _ : state) {
        // 以 n 為預估 bucket 大小, 避免反覆 rehash
        // (unordered_map 預設會在負載超標後自動擴增)
        std::unordered_map<int, int> umap;
        umap.reserve(n);

        for (size_t i = 0; i < n; i++) {
            benchmark::DoNotOptimize(
                umap.insert({ keys[i], static_cast<int>(i) })
            );
        }
        benchmark::ClobberMemory();
    }
}

// (2) 查找測試: Find
static void BM_UnorderedMap_Find(benchmark::State& state) {
    size_t n = static_cast<size_t>(state.range(0));
    auto keys = GenerateRandomKeys(n);

    std::unordered_map<int, int> umap;
    umap.reserve(n);
    for (size_t i = 0; i < n; i++) {
        umap.insert({ keys[i], static_cast<int>(i) });
    }

    for (auto _ : state) {
        for (int i = 0; i < 100; i++) {
            int idx = i % static_cast<int>(n);
            benchmark::DoNotOptimize(umap.find(keys[idx]));
        }
        benchmark::ClobberMemory();
    }
}

// (3) 高負載插入測試
static void BM_UnorderedMap_InsertHighLoad(benchmark::State& state) {
    size_t base_n = static_cast<size_t>(state.range(0));
    double target_load_pct = static_cast<double>(state.range(1)) / 100.0;
    size_t actual_insert_count = static_cast<size_t>(base_n * target_load_pct);

    auto keys = GenerateRandomKeys(actual_insert_count);

    for (auto _ : state) {
        // 預先 reserve base_n
        std::unordered_map<int, int> umap;
        umap.reserve(base_n);

        for (size_t i = 0; i < actual_insert_count; i++) {
            benchmark::DoNotOptimize(
                umap.insert({ keys[i], static_cast<int>(i) })
            );
        }
        benchmark::ClobberMemory();
    }
}


// ============================================================
//  註冊測試並執行 BENCHMARK_MAIN
// ============================================================

//// 針對 TinyPointerHashTable
//BENCHMARK(BM_TinyHashTable_Insert)->Arg(10000)->Arg(100000)->Arg(1000000);
//BENCHMARK(BM_TinyHashTable_Find)->Arg(10000)->Arg(100000)->Arg(1000000);
//
//// 測試不同高負載
//BENCHMARK(BM_TinyHashTable_InsertHighLoad)
//->Args({ 100000, 50 })
//->Args({ 100000, 80 })
//->Args({ 100000, 90 })
//->Args({ 100000, 95 })
//->Args({ 100000, 99 });

// 針對 std::unordered_map
//BENCHMARK(BM_UnorderedMap_Insert)->Arg(10000)->Arg(100000)->Arg(1000000);
//BENCHMARK(BM_UnorderedMap_Find)->Arg(10000)->Arg(100000)->Arg(1000000);
//
//BENCHMARK(BM_UnorderedMap_InsertHighLoad)
//->Args({ 100000, 50 })
//->Args({ 100000, 80 })
//->Args({ 100000, 90 })
//->Args({ 100000, 95 })
//->Args({ 100000, 99 });

//BENCHMARK(BM_FastFile_Write)->Arg(1024)->Arg(1024 * 1024);
//BENCHMARK(BM_STDFile_Write)->Arg(1024)->Arg(1024 * 1024);
//BENCHMARK(BM_FastFile_Read)->Arg(1024)->Arg(1024 * 1024);
//BENCHMARK(BM_STDFile_Read)->Arg(1024)->Arg(1024 * 1024);
//BENCHMARK(BM_FastFileWriteThreadPool)->Arg(2)->Arg(4)->Arg(8)->Arg(16)->Arg(32);
//BENCHMARK(BM_STDFileWriteThreadPool)->Arg(2)->Arg(4)->Arg(8)->Arg(16)->Arg(32);

//BENCHMARK(BM_ConvertFloatToInt24)->RangeMultiplier(2)->Range(256, 512);
//BENCHMARK(BM_ConvertInt8ToInt8SSE)->RangeMultiplier(2)->Range(256, 512);
//BENCHMARK(BM_ConvertFloatToInt24SSE)->RangeMultiplier(2)->Range(256, 512);
//BENCHMARK(BM_ConvertFloatToFloatSSE)->RangeMultiplier(2)->Range(256, 512);
//BENCHMARK(BM_ConvertFloatToInt32SSE)->RangeMultiplier(2)->Range(256, 512);

//BENCHMARK(BM_ThreadPoolBaseLine);
BENCHMARK(BM_ThreadPool)->Arg(2)->Arg(4)->Arg(8)->Arg(16)->Arg(32);
BENCHMARK(BM_StdThreadPool)->Arg(2)->Arg(4)->Arg(8)->Arg(16)->Arg(32);

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);

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
