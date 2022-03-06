#include <benchmark/benchmark.h>

#include <iostream>
#include <numeric>
#include <vector>
#include <unordered_map>
#include <execution>
#include <unordered_set>

#include <base/scopeguard.h>
#include <base/audiobuffer.h>
#include <base/threadpool.h>
#include <base/memory.h>
#include <base/rng.h>
#include <base/dataconverter.h>
#include <base/lrucache.h>
#include <base/stl.h>
#include <base/platform.h>
#include <base/uuid.h>

#include <player/fft.h>

#ifdef XAMP_OS_WIN
#include <base/win32/win32_threadpool.h>
#endif

using namespace xamp::base;
using namespace xamp::player;

#ifdef XAMP_OS_WIN

win32::ThreadPool& GetWin32ThreadPool() {
    static win32::ThreadPool win32_thread_pool("win32bench", 32, -1);
    return win32_thread_pool;
}

static void BM_Win32ThreadPool(benchmark::State& state) {
    Logger::GetInstance().GetLogger("win32bench")->set_level(spdlog::level::info);
    auto length = state.range(0);
    auto& thread_pool = GetWin32ThreadPool();
    std::vector<int> n(length);
    std::iota(n.begin(), n.end(), 1);
    std::atomic<int64_t> total;
    for (auto _ : state) {
        ParallelFor(n, [&total, &n](auto item) {
            total += item;
            }
        , thread_pool);
    }
}
#endif

static void BM_ThreadPool(benchmark::State& state) {
    static auto thread_pool = MakeThreadPool(kPlaybackThreadPoolLoggerName,
        std::thread::hardware_concurrency(),
        -1,
        ThreadPriority::NORMAL);
    auto length = state.range(0);
    std::vector<int> n(length);
    std::iota(n.begin(), n.end(), 1);
    std::atomic<int64_t> total;
    for (auto _ : state) {
        ParallelFor(n, [&total, &n](auto item) {
            total += item;
        }
        , *thread_pool);
    }
}

template <typename C, typename Func>
void StdAsyncParallelFor(C& items, Func&& f, size_t batches = 4) {
    auto begin = std::begin(items);
    auto size = std::distance(begin, std::end(items));

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
}

static void BM_async_pool(benchmark::State& state) {
    auto length = state.range(0);
    std::vector<int> n(length);
    std::iota(n.begin(), n.end(), 1);
    std::atomic<int64_t> total;
    for (auto _ : state) {
        StdAsyncParallelFor(n, [&total, &n](auto item) {
            for (auto i = 0; i < 100; ++i) {
                total += item;
            }
            });
    }
}

static void BM_std_for_each_par(benchmark::State& state) {
    auto length = state.range(0);
    std::vector<int> n(length);
    std::iota(n.begin(), n.end(), 1);
    std::atomic<int64_t> total;
    for (auto _ : state) {
        std::for_each(std::execution::par_unseq,
            n.begin(),
            n.end(),
            [&total](auto&& item)
            {
                for (auto i = 0; i < 100; ++i) {
                    total += item;
                }
            });
    }
}

static void BM_Xoshiro256StarStarRandom(benchmark::State& state) {
    Xoshiro256StarStarEngine engine;
    for (auto _ : state) {
        size_t n = std::uniform_int_distribution<int64_t>(INT64_MIN, INT64_MAX)(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(float));
}

static void BM_Xoshiro256PlusRandom(benchmark::State& state) {
    Xoshiro256PlusEngine engine;
    for (auto _ : state) {
        size_t n = std::uniform_int_distribution<int64_t>(INT64_MIN, INT64_MAX)(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(float));
}

static void BM_Xoshiro256PlusPlusRandom(benchmark::State& state) {
    Xoshiro256PlusPlusEngine engine;
    for (auto _ : state) {
        size_t n = std::uniform_int_distribution<int64_t>(INT64_MIN, INT64_MAX)(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(float));
}

static void BM_default_random_engine(benchmark::State& state) {
    std::random_device rd;
    std::default_random_engine engine(rd());
    for (auto _ : state) {
        size_t n = std::uniform_int_distribution<int64_t>(INT64_MIN, INT64_MAX)(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(float));
}

static void BM_PRNG(benchmark::State& state) {
    PRNG prng;
    for (auto _ : state) {
        size_t n = prng.NextInt64(INT64_MIN, INT64_MAX);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(float));
}

static void BM_PRNG_GetInstance(benchmark::State& state) {
    for (auto _ : state) {
        size_t n = PRNG::GetInstance().NextInt64(INT64_MIN, INT64_MAX);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(float));
}

static void BM_ConvertToInt2432Avx(benchmark::State& state) {
    auto length = state.range(0);

    auto output = MakeAlignedArray<int>(length);
    auto input = PRNG::GetInstance().GetRandomFloat(length, -1.0, 1.0);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            ConvertToInt2432(output.get(), input.get(), ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_ConvertToInt2432(benchmark::State& state) {
    auto length = state.range(0);

    auto input = PRNG::GetInstance().GetRandomFloat(length, -1.0, 1.0);
    auto output = MakeAlignedArray<int>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            ConvertToInt2432Bench(output.get(), input.get(), ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_InterleavedToPlanarConvertToInt32_AVX(benchmark::State& state) {
    auto length = state.range(0);

    auto input = PRNG::GetInstance().GetRandomFloat(length, -1.0, 1.0);
    auto output = MakeAlignedArray<int>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    for (auto _ : state) {
        InterleaveToPlanar<float, int32_t>::Convert(input.get(),
            output.get(),
            output.get() + (length / 2),
            length);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_InterleavedToPlanarConvertToInt32(benchmark::State& state) {
    auto length = state.range(0);

    auto input = PRNG::GetInstance().GetRandomFloat(length, -1.0, 1.0);
    auto output = MakeAlignedArray<int>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);
    output_format.SetPackedFormat(PackedFormat::PLANAR);

    const auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::PLANAR,
            PackedFormat::INTERLEAVED>::Convert(
                output.get(),
                input.get(),
                ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_FastMemcpy(benchmark::State& state) {
    auto length = state.range(0);

    auto src = MakeAlignedArray<int8_t>(length);
    auto dest = MakeAlignedArray<int8_t>(length);

    for (auto _ : state) {
        MemoryCopy(dest.get(), src.get(), length);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length);
}

static void BM_StdtMemcpy(benchmark::State& state) {
    auto length = state.range(0);

    auto src = MakeAlignedArray<int8_t>(length);
    auto dest = MakeAlignedArray<int8_t>(length);

    for (auto _ : state) {
        std::memcpy(dest.get(), src.get(), length);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length);
}

static void BM_FastMemset(benchmark::State& state) {
    auto length = state.range(0);

    auto src = MakeAlignedArray<int8_t>(length);
    auto dest = MakeAlignedArray<int8_t>(length);

    for (auto _ : state) {
        MemorySet(dest.get(), 0, length);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length);
}

static void BM_StdtMemset(benchmark::State& state) {
    auto length = state.range(0);

    auto src = MakeAlignedArray<int8_t>(length);
    auto dest = MakeAlignedArray<int8_t>(length);

    for (auto _ : state) {
        std::memset(dest.get(), 0, length);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length);
}

static void BM_FindRobinHoodHashMap(benchmark::State& state) {
    HashMap<int64_t, int64_t> map;
    for (auto _ : state) {
        map.emplace(std::make_pair(PRNG::GetInstance().NextInt64(), PRNG::GetInstance().NextInt64()));
        (void)map.find(PRNG::GetInstance().NextInt64());
    }
}

static void BM_unordered_map(benchmark::State& state) {
    std::unordered_map<int64_t, int64_t> map;
    for (auto _ : state) {
        map.emplace(std::make_pair(PRNG::GetInstance().NextInt64(), PRNG::GetInstance().NextInt64()));
        (void)map.find(PRNG::GetInstance().NextInt64());
    }
}

static void BM_FindRobinHoodHashSet(benchmark::State& state) {
    HashSet<int64_t> map;
    for (auto _ : state) {
        map.emplace(PRNG::GetInstance().NextInt64());
        (void)map.find(PRNG::GetInstance().NextInt64());
    }
}

static void BM_unordered_set(benchmark::State& state) {
    std::unordered_set<int64_t> map;
    for (auto _ : state) {
        map.emplace(PRNG::GetInstance().NextInt64());
        (void)map.find(PRNG::GetInstance().NextInt64());
    }
}

static void BM_LruCache(benchmark::State& state) {
    auto length = state.range(0);

    LruCache<int64_t, int64_t> cache;

    for (auto i = 0; i < length; ++i) {
        cache.AddOrUpdate(PRNG::GetInstance().NextInt64(1, 1000),\
            PRNG::GetInstance().NextInt64(1, 1000));
    }
    
    for (auto _ : state) {
        cache.Find(PRNG::GetInstance().NextInt64(1, 1000));
    }
}

static void BM_FFT(benchmark::State& state) {
    auto length = state.range(0);

    auto input = PRNG::GetInstance().GetRandomFloat(length, -1.0, 1.0);
    FFT fft;
    fft.Init(length);
    for (auto _ : state) {
        auto& result = fft.Forward(input.get(), length);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_UuidParse(benchmark::State& state) {
    const auto uuid_str = MakeUuidString();

    for (auto _ : state) {
        auto result = Uuid::FromString(uuid_str);
        benchmark::DoNotOptimize(result);
    }
}

//BENCHMARK(BM_UuidParse);
//BENCHMARK(BM_Xoshiro256StarStarRandom);
//BENCHMARK(BM_Xoshiro256PlusRandom);
//BENCHMARK(BM_Xoshiro256PlusPlusRandom);
//BENCHMARK(BM_default_random_engine);
//BENCHMARK(BM_PRNG);
//BENCHMARK(BM_PRNG_GetInstance);
//
//BENCHMARK(BM_unordered_set);
//BENCHMARK(BM_FindRobinHoodHashSet);
//BENCHMARK(BM_unordered_map);
//BENCHMARK(BM_FindRobinHoodHashMap);
//BENCHMARK(BM_FastMemset)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_StdtMemset)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_FastMemcpy)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_StdtMemcpy)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_ConvertToInt2432Avx)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_ConvertToInt2432)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_InterleavedToPlanarConvertToInt32_AVX)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_InterleavedToPlanarConvertToInt32)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_FFT)->RangeMultiplier(2)->Range(4096, 8 << 12);

BENCHMARK(BM_ThreadPool)->RangeMultiplier(2)->Range(8, 8 << 16);
//BENCHMARK(BM_async_pool)->RangeMultiplier(2)->Range(8, 8 << 16);
//BENCHMARK(BM_std_for_each_par)->RangeMultiplier(2)->Range(8, 8 << 16);
#ifdef XAMP_OS_WIN
//BENCHMARK(BM_Win32ThreadPool)->RangeMultiplier(2)->Range(8, 8 << 16);
#endif


int main(int argc, char** argv) {
    //std::cin.get();

    Logger::GetInstance()
        .AddDebugOutputLogger()
        .AddFileLogger("xamp.log")
        .GetLogger("xamp");

    XAMP_ON_SCOPE_EXIT(
        Logger::GetInstance().Shutdown();
    );

    // For debug use!
    XAMP_LOG_DEBUG("Logger init success.");

    FFT::LoadFFTLib();

    Logger::GetInstance().GetLogger(kPlaybackThreadPoolLoggerName)->set_level(spdlog::level::info);

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return -1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
    //std::cin.get();
}
