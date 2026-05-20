#include <benchmark/benchmark.h>

#include <base/trackinfo.h>
#include <base/threadpoolexecutor.h>
#include <base/rng.h>
#include <base/dataconverter.h>
#include <base/lrucache.h>
#include <base/stl.h>
#include <base/platform.h>
#include <base/uuid.h>
#include <base/shared_singleton.h>
#include <base/singleton.h>

#include <base/logger.h>
#include <base/executor.h>

#include <stream/stream.h>
#include <stream/api.h>
#include <stream/ifileencoder.h>

#include <player/api.h>

#include <iostream>
#include <numeric>
#include <vector>
#include <unordered_map>
#include <execution>
#include <unordered_set>

#include <thread_pool/thread_pool.h>
#include <xsimd/xsimd.hpp>
#include <base/dll.h>
#include <base/fs.h>

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

void ConvertInt8ToInt8_xsimd(const int8_t* input, int8_t* left_ptr, int8_t* right_ptr, size_t frames) {
    constexpr size_t BATCH_SIZE = xsimd::batch<int8_t>::size;
    using batch_type = xsimd::batch<int8_t>;
    size_t i = 0;

    // 每個 frame 包含左右聲道各一個 int8_t，因此每個 batch 處理 BATCH_SIZE / 2 個 frames
    size_t frames_per_batch = BATCH_SIZE / 2;

    while (frames >= frames_per_batch) {
        // 載入交錯的音訊資料
        batch_type data = batch_type::load_unaligned(input);

        // 使用 zip_lo 和 zip_hi 分離左右聲道
        auto left_batch = xsimd::zip_lo(data, data);
        auto right_batch = xsimd::zip_hi(data, data);

        // 儲存左右聲道資料
        left_batch.store_unaligned(left_ptr);
        right_batch.store_unaligned(right_ptr);

        input += BATCH_SIZE;
        left_ptr += frames_per_batch;
        right_ptr += frames_per_batch;
        frames -= frames_per_batch;
        i += frames_per_batch;
    }

    // 處理剩餘不足一個 batch 的 frames
    for (; frames > 0; --frames) {
        *left_ptr++ = *input++;
        *right_ptr++ = *input++;
    }
}

void ConvertFloatToInt24_xsimd(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames) {
    using batch = xsimd::batch<float>;
    using ibatch = xsimd::batch<int32_t>;
    constexpr size_t simd_size = batch::size; // 通常為 4/8/16，依平台

    const float* in = input;
    int32_t* lptr = left_ptr;
    int32_t* rptr = right_ptr;

    size_t i = 0;
    batch scale = batch(kFloat24Scale);

    // 一次處理 simd_size 個 frame（每 frame 是 L/R 交錯的 2 值）
    for (; i + simd_size <= frames; i += simd_size) {
        // 讀入 2 * simd_size 個 float，代表 simd_size 個 frame 的 L/R 交錯資料
        batch interleaved1 = batch::load_unaligned(in);
        batch interleaved2 = batch::load_unaligned(in + simd_size);

        // 使用 zip_lo 和 zip_hi 分離左右聲道
        auto left = xsimd::zip_lo(interleaved1, interleaved2);
        auto right = xsimd::zip_hi(interleaved1, interleaved2);

        // 乘以縮放因子
        left = left * scale;
        right = right * scale;

        // 轉換為 int32 並左移 8 位
        ibatch left_i32 = xsimd::to_int(left) << 8;
        ibatch right_i32 = xsimd::to_int(right) << 8;

        // 儲存結果
        left_i32.store_unaligned(lptr);
        right_i32.store_unaligned(rptr);

        in += simd_size * 2;
        lptr += simd_size;
        rptr += simd_size;
    }

    // 處理剩餘不足 simd_size 的 frame
    for (; i < frames; i++) {
        float L = in[0] * kFloat24Scale;
        float R = in[1] * kFloat24Scale;
        *lptr++ = static_cast<int32_t>(L) << 8;
        *rptr++ = static_cast<int32_t>(R) << 8;
        in += 2;
    }
}

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

//static void BM_DpThreadPool(benchmark::State& state) {
//    const size_t max_thread = state.range(0);
//    dp::thread_pool<std::function<void()>> pool(max_thread);
//    PRNG prng;
//    std::atomic<int64_t> prime_count{ 0 };
//
//    for (auto _ : state) {
//        std::vector<std::future<void>> results;
//        results.reserve(max_thread);
//        for (auto i = 0; i < max_thread; ++i) {
//            auto number = prng.NextUInt32();
//            auto task_lambda = [&prime_count, number]() {
//                if (IsPrime(number)) {
//                    ++prime_count;
//                }
//                };
//            results.push_back(pool.enqueue(task_lambda));
//        }
//        for (auto& fut : results) {
//            fut.wait();
//        }
//        benchmark::DoNotOptimize(prime_count.load());
//    }
//}

static void BM_ThreadPool(benchmark::State& state) {
    const size_t max_thread = state.range(0);

    auto thread_pool = ThreadPoolBuilder::MakeThreadPool(
        XAMP_LOG_NAME(BM_ThreadPool),
        max_thread,
        max_thread / 2,
        ThreadPriority::PRIORITY_NORMAL);

    XampLoggerFactory.GetLogger(XAMP_LOG_NAME(BM_ThreadPool))
        ->SetLevel(LOG_LEVEL_OFF);

    PRNG prng;
    std::atomic<int64_t> prime_count{ 0 };
    std::vector<uint32_t> n(max_thread);
    std::generate(n.begin(), n.end(), [&prng]() {
        return prng.NextUInt32();
        });
    auto task_lambda = [&prime_count](auto number) {
        if (IsPrime(number)) {
            ++prime_count;
        }
    };

    for (auto _ : state) {
        Executor::ParallelForSimple(thread_pool, n, task_lambda);
        benchmark::DoNotOptimize(prime_count.load());
    }
}

static void BM_StdThreadPool(benchmark::State& state) {
    auto std_parallel_for = [](auto& items, auto&& f) {
        auto begin = std::begin(items);
        size_t size = std::distance(begin, std::end(items));

        std::vector<std::shared_future<void>> futures;
        futures.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            futures.push_back(std::async(std::launch::async, [f, begin, i]() -> void {
                f(*(begin + i));
                }));
            ++i;
        }
        for (auto& ff : futures) {
            ff.wait();
        }
        };
    const size_t max_thread = state.range(0);
    std::vector<uint32_t> n(max_thread);
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

static void BM_ConvertFloatToInt16SSE(benchmark::State& state) {
    const size_t frames = static_cast<size_t>(state.range(0));
    PRNG prng;
    auto input = prng.NextSingles(2 * frames);
    std::vector<int16_t> left(frames), right(frames);
    for (auto _ : state) {
        ConvertFloatToInt16SSE(input.data(), left.data(), right.data(), frames);
        benchmark::DoNotOptimize(left);
        benchmark::DoNotOptimize(right);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * frames);
}

static void BM_ConvertFloatToInt16(benchmark::State& state) {
    const size_t frames = static_cast<size_t>(state.range(0));
    PRNG prng;
    auto input = prng.NextSingles(2 * frames);
    std::vector<int16_t> left(frames), right(frames);
    for (auto _ : state) {
        ConvertFloatToInt16(input.data(), left.data(), right.data(), frames);
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

static void BM_Log10Fast(benchmark::State& state) {
    PRNG prng;
    for (auto _ : state) {
        benchmark::DoNotOptimize(log10f_fast(prng.NextSingle()));
    }
}

static void BM_StdLog10(benchmark::State& state) {
    PRNG prng;
    for (auto _ : state) {
        benchmark::DoNotOptimize(std::log10f(prng.NextSingle()));
    }
}

static void BM_TemporaryFile_Write(benchmark::State& state) {
    PRNG prng;
    TemporaryFile file;
    auto tempstr = prng.GetRandomString(64 * 1024);
    for (auto _ : state) {
        file.Write(tempstr.c_str(), tempstr.length(), 1);
    }
}

static void BM_CTemporaryFile_Write(benchmark::State& state) {
    PRNG prng;
    CTemporaryFile file;
    auto tempstr = prng.GetRandomString(64 * 1024);
    for (auto _ : state) {
        file.Write(tempstr.c_str(), tempstr.length(), 1);
    }
}

static void BM_TemporaryFile_Read(benchmark::State& state) {
    PRNG prng;
    TemporaryFile file;

    for (size_t i = 0; i < 10 * 64 * 1024; i += 64 * 1024) {
        auto tempstr = prng.GetRandomString(64 * 1024);
        file.Write(tempstr.c_str(), tempstr.length(), 1);
    }
    std::vector<char> buffer(4 * 1024);
    for (auto _ : state) {
        file.Read(buffer.data(), buffer.size(), 1);
    }
}

static void BM_CTemporaryFile_Read(benchmark::State& state) {
    PRNG prng;
    CTemporaryFile file;

    for (size_t i = 0; i < 10 * 64 * 1024; i += 64 * 1024) {
        auto tempstr = prng.GetRandomString(64 * 1024);
        file.Write(tempstr.c_str(), tempstr.length(), 1);
    }    
    std::vector<char> buffer(4 * 1024);
    for (auto _ : state) {        
        file.Read(buffer.data(), buffer.size(), 1);
    }
}

BENCHMARK(BM_StdLog10);
BENCHMARK(BM_Log10Fast);

BENCHMARK(BM_TemporaryFile_Write);
BENCHMARK(BM_CTemporaryFile_Write);
BENCHMARK(BM_TemporaryFile_Read);
BENCHMARK(BM_CTemporaryFile_Read);

//BENCHMARK(BM_V1_CreateAndDestroy);
//BENCHMARK(BM_V2_CreateAndDestroy);
//
//BENCHMARK(BM_V1_CreateAndInvoke);
//BENCHMARK(BM_V2_CreateAndInvoke);
//
//BENCHMARK(BM_V1_MoveOperation);
//BENCHMARK(BM_V2_MoveOperation);

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

BENCHMARK(BM_ThreadPoolBaseLine)->Arg(4)->Arg(8)->Arg(16)->Arg(32);
BENCHMARK(BM_ThreadPool)->Arg(4)->Arg(8)->Arg(16)->Arg(32);
//BENCHMARK(BM_DpThreadPool)->Arg(4)->Arg(8)->Arg(16)->Arg(32);
//BENCHMARK(BM_StdThreadPool)->Arg(4)->Arg(8)->Arg(16)->Arg(32);

BENCHMARK(BM_ConvertFloatToInt24)->RangeMultiplier(2)->Range(256, 512);
BENCHMARK(BM_ConvertFloatToInt24SSE)->RangeMultiplier(2)->Range(256, 512);

BENCHMARK(BM_ConvertFloatToInt16)->RangeMultiplier(2)->Range(256, 512);
BENCHMARK(BM_ConvertFloatToInt16SSE)->RangeMultiplier(2)->Range(256, 512);

BENCHMARK(BM_ConvertFloatToFloatSSE)->RangeMultiplier(2)->Range(256, 512);
BENCHMARK(BM_ConvertFloatToInt32SSE)->RangeMultiplier(2)->Range(256, 512);
BENCHMARK(BM_ConvertInt8ToInt8SSE)->RangeMultiplier(2)->Range(256, 512);

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
