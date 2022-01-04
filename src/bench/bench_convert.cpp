#include <benchmark/benchmark.h>

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>

#include <base/scopeguard.h>
#include <base/audiobuffer.h>
#include <base/threadpool.h>
#include <base/memory.h>
#include <base/rng.h>
#include <base/fastmutex.h>
#include <base/dataconverter.h>
#include <base/lrucache.h>
#include <base/stl.h>

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

    std::vector<std::shared_future<void>> tasks;
    for (auto _ : state) {
        for (auto i = 0; i < 8; ++i) {
            tasks.push_back(GetWin32ThreadPool().Spawn([](size_t thread_index) {}));
            if (i % 8 == 0) {
                for (auto& t : tasks) {
                    if (t.valid()) {
                        try {
                            t.get();
                        }
                        catch (std::exception const& e) {
                            std::cout << e.what() << std::endl;
                        }
                    }
                }
                tasks.clear();
            }
        }
        for (auto& t : tasks) {
            if (t.valid()) {
                try {
                    t.get();
                }
                catch (std::exception const &e) {
                    std::cout << e.what() << std::endl;
                }
            }
        }
        tasks.clear();
    }
}
#endif

static void BM_ThreadPool(benchmark::State& state) {
    auto& thread_pool = PlaybackThreadPool();
    std::vector<std::shared_future<void>> tasks;
    for (auto _ : state) {
        for (auto i =0 ; i < 8; ++i) {
            tasks.push_back(thread_pool.Spawn([](size_t thread_index) {}));
            if (i % 8 == 0) {
	            for (auto &t: tasks) {
                    if (t.valid()) {
                        t.get();
                    }
	            }
                tasks.clear();
            }
        }
        for (auto& t : tasks) {
            if (t.valid()) {
                t.get();
            }
        }
        tasks.clear();
    }
}

static void BM_async(benchmark::State& state) {
    std::vector<std::future<void>> tasks;
    for (auto _ : state) {        
        for (auto i = 0; i < 8; ++i) {
            tasks.push_back(std::async(std::launch::async, []() {}));
            if (i % 8 == 0) {
                for (auto& t : tasks) {
                    if (t.valid()) {
                        t.get();
                    }
                }
                tasks.clear();
            }
        }
        for (auto& t : tasks) {
            if (t.valid()) {
                t.get();
            }
        }
        tasks.clear();
    }
}

static void BM_Xoshiro256StarStarRandom(benchmark::State& state) {
    Xoshiro256StarStarEngine engine;
    std::uniform_int_distribution<int32_t> unif(INT_MIN, INT_MAX);
    for (auto _ : state) {
        size_t n = unif(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(float));
}

static void BM_Xoshiro256PlusRandom(benchmark::State& state) {
    Xoshiro256PlusEngine engine;
    std::uniform_int_distribution<int32_t> unif(INT_MIN, INT_MAX);
    for (auto _ : state) {
        size_t n = unif(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(float));
}

static void BM_Xoshiro256PlusPlusRandom(benchmark::State& state) {
    Xoshiro256PlusPlusEngine engine;
    std::uniform_int_distribution<int32_t> unif(INT_MIN, INT_MAX);
    for (auto _ : state) {
        size_t n = unif(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(float));
}

static void BM_default_random_engine(benchmark::State& state) {
    std::random_device rd;
    std::default_random_engine engine(rd());
    std::uniform_int_distribution<int32_t> unif(INT_MIN, INT_MAX);
    for (auto _ : state) {
        size_t n = unif(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(float));
}

static void BM_ConvertToInt2432Avx(benchmark::State& state) {
    auto length = state.range(0);

    auto output = MakeAlignedArray<int>(length);
    auto input = PRNG::GetRandomFloat(length, -1.0, 1.0);

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

    auto input = PRNG::GetRandomFloat(length, -1.0, 1.0);
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

    auto input = PRNG::GetRandomFloat(length, -1.0, 1.0);
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

    auto input = PRNG::GetRandomFloat(length, -1.0, 1.0);
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
    HashMap<int32_t, int32_t> map;
    for (auto _ : state) {
        map.emplace(std::make_pair(PRNG::NextInt(), PRNG::NextInt()));
        (void)map.find(PRNG::NextInt());
    }
}

static void BM_unordered_map(benchmark::State& state) {
    std::unordered_map<std::int32_t, int32_t> map;
    for (auto _ : state) {
        map.emplace(std::make_pair(PRNG::NextInt(), PRNG::NextInt()));
        (void)map.find(PRNG::NextInt());
    }
}

static void BM_FindRobinHoodHashSet(benchmark::State& state) {
    HashSet<int32_t> map;
    for (auto _ : state) {
        map.emplace(PRNG::NextInt());
        (void)map.find(PRNG::NextInt());
    }
}

static void BM_unordered_set(benchmark::State& state) {
    std::unordered_set<std::int32_t> map;
    for (auto _ : state) {
        map.emplace(PRNG::NextInt());
        (void)map.find(PRNG::NextInt());
    }
}

static void BM_LruCache(benchmark::State& state) {
    auto length = state.range(0);

    LruCache<int32_t, int32_t> cache;

    for (auto i = 0; i < length; ++i) {
        cache.AddOrUpdate(PRNG::NextInt(1, 1000), PRNG::NextInt(1, 1000));
    }
    
    for (auto _ : state) {
        cache.Find(PRNG::NextInt(1, 1000));
    }
}

static void BM_FFT(benchmark::State& state) {
    auto length = state.range(0);

    auto input = PRNG::GetRandomFloat(length, -1.0, 1.0);
    FFT fft;
    fft.Init(length);
    for (auto _ : state) {
        auto& result = fft.Forward(input.get(), length);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BM_FFT)->RangeMultiplier(2)->Range(4096, 8 << 12);
BENCHMARK(BM_async);
#ifdef XAMP_OS_WIN
BENCHMARK(BM_Win32ThreadPool);
#endif
BENCHMARK(BM_ThreadPool);

BENCHMARK(BM_Xoshiro256StarStarRandom);
BENCHMARK(BM_Xoshiro256PlusRandom);
BENCHMARK(BM_Xoshiro256PlusPlusRandom);
BENCHMARK(BM_default_random_engine);
BENCHMARK(BM_unordered_set);
BENCHMARK(BM_FindRobinHoodHashSet);
BENCHMARK(BM_unordered_map);
BENCHMARK(BM_FindRobinHoodHashMap);
BENCHMARK(BM_LruCache)->RangeMultiplier(2)->Range(4096, 8 << 16);
BENCHMARK(BM_FastMemset)->RangeMultiplier(2)->Range(4096, 8 << 10);
BENCHMARK(BM_StdtMemset)->RangeMultiplier(2)->Range(4096, 8 << 10);
BENCHMARK(BM_FastMemcpy)->RangeMultiplier(2)->Range(4096, 8 << 10);
BENCHMARK(BM_StdtMemcpy)->RangeMultiplier(2)->Range(4096, 8 << 10);
BENCHMARK(BM_ConvertToInt2432Avx)->RangeMultiplier(2)->Range(4096, 8 << 10);
BENCHMARK(BM_ConvertToInt2432)->RangeMultiplier(2)->Range(4096, 8 << 10);
BENCHMARK(BM_InterleavedToPlanarConvertToInt32_AVX)->RangeMultiplier(2)->Range(4096, 8 << 10);
BENCHMARK(BM_InterleavedToPlanarConvertToInt32)->RangeMultiplier(2)->Range(4096, 8 << 10);

int main(int argc, char** argv) {
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
