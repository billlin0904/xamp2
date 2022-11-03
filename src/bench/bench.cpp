#include <benchmark/benchmark.h>

#include <iostream>
#include <numeric>
#include <vector>
#include <unordered_map>
#include <execution>
#include <unordered_set>

#include <base/scopeguard.h>
#include <base/metadata.h>
#include <base/lifoqueue.h>
#include <base/audiobuffer.h>
#include <base/threadpool.h>
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
#include <base/ppl.h>
#include <base/chachaengine.h>

#include <stream/api.h>
#include <stream/fft.h>

#include <player/api.h>

#ifdef XAMP_OS_WIN
#include <base/simd.h>
#else
#include <uuid/uuid.h>
#endif

using namespace xamp::player;
using namespace xamp::base;
using namespace xamp::stream;

static void BM_LeastLoadThreadPool(benchmark::State& state) {
    const auto thread_pool = MakeThreadPool(
        "BM_LeastLoadThreadPool",
        ThreadPriority::NORMAL,
        std::thread::hardware_concurrency(),
        kDefaultAffinityCpuCore,
        TaskSchedulerPolicy::LEAST_LOAD_POLICY);
    const auto length = state.range(0);
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

static void BM_RoundRubinThreadPool(benchmark::State& state) {
	const auto thread_pool = MakeThreadPool(
        "BM_RoundRubinThreadPool",
        ThreadPriority::NORMAL,
        std::thread::hardware_concurrency(),
        kDefaultAffinityCpuCore,
        TaskSchedulerPolicy::ROUND_ROBIN_POLICY);
    const auto length = state.range(0);
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

static void BM_ChildStealPolicyRandomThreadPool(benchmark::State& state) {
    const auto thread_pool = MakeThreadPool(
        "BM_ChildStealPolicyRandomThreadPool",
        ThreadPriority::NORMAL,
        std::thread::hardware_concurrency(),
        kDefaultAffinityCpuCore,
        TaskSchedulerPolicy::RANDOM_POLICY,
        TaskStealPolicy::CHILD_STEALING_POLICY);
    const auto length = state.range(0);
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

static void BM_ContinuationStealPolicyRandomThreadPool(benchmark::State& state) {
    const auto thread_pool = MakeThreadPool(
        "BM_ContinuationStealPolicyRandomThreadPool",
        ThreadPriority::NORMAL,
        std::thread::hardware_concurrency(),
        kDefaultAffinityCpuCore,
        TaskSchedulerPolicy::ROUND_ROBIN_POLICY,
        TaskStealPolicy::CONTINUATION_STEALING_POLICY);
    const auto length = state.range(0);
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
            total += item;
            });
    }
}

#ifdef XAMP_OS_WIN
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
#endif

static void BM_Xoshiro256StarStarRandom(benchmark::State& state) {
    Xoshiro256StarStarEngine engine;
    for (auto _ : state) {
        size_t n = std::uniform_int_distribution<int64_t>(INT32_MIN, INT32_MAX)(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(int64_t));
}

static void BM_Xoshiro256PlusRandom(benchmark::State& state) {
    Xoshiro256PlusEngine engine;
    for (auto _ : state) {
        size_t n = std::uniform_int_distribution<int32_t>(INT32_MIN, INT32_MAX)(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(int64_t));
}

static void BM_Xoshiro256PlusPlusRandom(benchmark::State& state) {
    Xoshiro256PlusPlusEngine engine;
    for (auto _ : state) {
        size_t n = std::uniform_int_distribution<int32_t>(INT32_MIN, INT32_MAX)(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(int64_t));
}

static void BM_ChaCha20Random(benchmark::State& state) {
    ChaChaEngine engine;
    for (auto _ : state) {
        size_t n = std::uniform_int_distribution<int32_t>(INT32_MIN, INT32_MAX)(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(int64_t));
}

static void BM_default_random_engine(benchmark::State& state) {
    std::random_device rd;
    std::default_random_engine engine(rd());
    for (auto _ : state) {
        size_t n = std::uniform_int_distribution<int32_t>(INT32_MIN, INT32_MAX)(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(int64_t));
}

static void BM_PRNG(benchmark::State& state) {
    PRNG prng;
    for (auto _ : state) {
        size_t n = prng.NextInt64();
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(int64_t));
}

static void BM_PRNG_GetInstance(benchmark::State& state) {
    for (auto _ : state) {
        size_t n = Singleton<PRNG>::GetInstance().NextInt64();
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(int64_t));
}

static void BM_PRNG_SharedGetInstance(benchmark::State& state) {
    for (auto _ : state) {
        size_t n = SharedSingleton<PRNG>::GetInstance().NextInt64();
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(int64_t));
}

static void BM_ConvertToInt2432Avx(benchmark::State& state) {
    auto length = state.range(0);

    auto output = Vector<int>(length);
    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            ConvertToInt2432(output.data(), input.data(), ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_ConvertToInt2432(benchmark::State& state) {
    auto length = state.range(0);

    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);
    auto output = Vector<int>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            ConvertToInt2432Bench(output.data(), input.data(), ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_InterleavedToPlanarConvertToInt32_AVX(benchmark::State& state) {
    auto length = state.range(0);

    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);
    auto output = Vector<int>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    for (auto _ : state) {
        InterleaveToPlanar<float, int32_t>::Convert(input.data(),
            output.data(),
            output.data() + (length / 2),
            length);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_InterleavedToPlanarConvertToInt32(benchmark::State& state) {
    auto length = state.range(0);

    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);
    auto output = Vector<int>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);
    output_format.SetPackedFormat(PackedFormat::PLANAR);

    const auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::PLANAR,
            PackedFormat::INTERLEAVED>::Convert(
                output.data(),
                input.data(),
                ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_InterleavedToPlanarConvertToInt8_AVX(benchmark::State& state) {
    auto length = state.range(0);

    auto input = Singleton<PRNG>::GetInstance().NextBytes(length);
    auto output = Vector<int8_t>(length * 2);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    for (auto _ : state) {
        InterleaveToPlanar<int8_t, int8_t>::Convert(input.data(),
            output.data(),
            output.data() + (length / 2),
            length);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(int8_t));
}

static void BM_InterleavedToPlanarConvertToInt8(benchmark::State& state) {
    auto length = state.range(0);

    auto input = Singleton<PRNG>::GetInstance().NextBytes(length);
    auto output = Vector<int8_t>(length * 2);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    input_format.SetPackedFormat(PackedFormat::PLANAR);
    output_format.SetPackedFormat(PackedFormat::PLANAR);

    const auto ctx = MakeConvert(input_format, output_format, length / 2);


    for (auto _ : state) {
        DataConverter<PackedFormat::PLANAR, PackedFormat::PLANAR>::Convert(
            output.data(),
            input.data(),
            ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(int8_t));
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
        map.emplace(std::make_pair(SharedSingleton<PRNG>::GetInstance().NextInt64(), Singleton<PRNG>::GetInstance().NextInt64()));
        (void)map.find(SharedSingleton<PRNG>::GetInstance().NextInt64());
    }
}

static void BM_unordered_map(benchmark::State& state) {
    std::unordered_map<int64_t, int64_t> map;
    for (auto _ : state) {
        map.emplace(std::make_pair(SharedSingleton<PRNG>::GetInstance().NextInt64(), Singleton<PRNG>::GetInstance().NextInt64()));
        (void)map.find(SharedSingleton<PRNG>::GetInstance().NextInt64());
    }
}

static void BM_FindRobinHoodHashSet(benchmark::State& state) {
    HashSet<int64_t> map;
    for (auto _ : state) {
        map.emplace(SharedSingleton<PRNG>::GetInstance().NextInt64());
        (void)map.find(SharedSingleton<PRNG>::GetInstance().NextInt64());
    }
}

static void BM_unordered_set(benchmark::State& state) {
    std::unordered_set<int64_t> map;
    for (auto _ : state) {
        map.emplace(SharedSingleton<PRNG>::GetInstance().NextInt64());
        (void)map.find(SharedSingleton<PRNG>::GetInstance().NextInt64());
    }
}

static void BM_FowardListSort(benchmark::State& state) {
    auto length = state.range(0);
    ForwardList<Metadata> list;

    for (auto i = 0; i < length; ++i) {
        Metadata metadata;
        metadata.track = SharedSingleton<PRNG>::GetInstance().NextInt64();
        list.push_front(metadata);
    }

    for (auto _ : state) {
        list.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
            });
    }
}

static void BM_VectorSort(benchmark::State& state) {
    auto length = state.range(0);
    std::vector<Metadata> list;

    for (auto i = 0; i < length; ++i) {
        Metadata metadata;
        metadata.track = SharedSingleton<PRNG>::GetInstance().NextInt64();
        list.push_back(metadata);
    }

    for (auto _ : state) {
        std::stable_sort(list.begin(), list.end(),
            [](const auto& first, const auto& last) {
                return first.track < last.track;
            });
    }
}

static void BM_LruCache_Find(benchmark::State& state) {
    auto length = state.range(0);

    LruCache<int64_t, int64_t> cache;

    for (auto i = 0; i < length; ++i) {
        cache.Add(
            SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000),\
            SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000));
    }
    
    for (auto _ : state) {
        cache.Find(SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000));
    }
}

static void BM_LruCache_Add(benchmark::State& state) {
    auto length = state.range(0);

    LruCache<int64_t, int64_t> cache;

    for (auto _ : state) {
        cache.Add(
            SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000), \
            SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000));
    }
}

static void BM_LruCache_AddOrUpdate(benchmark::State& state) {
    auto length = state.range(0);

    LruCache<int64_t, int64_t> cache;

    for (auto _ : state) {
        cache.AddOrUpdate(
            SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000), \
            SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000));
    }
}

static void BM_FFT(benchmark::State& state) {
    auto length = state.range(0);

    auto input = SharedSingleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);
    FFT fft;
    fft.Init(length);
    for (auto _ : state) {
        auto& result = fft.Forward(input.data(), length);
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
}

static void BM_Builtin_UuidParse(benchmark::State& state) {
    const auto uuid_str = MakeUuidString();
    for (auto _ : state) {
        auto result = ParseUuidString(uuid_str);
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
}

static void BM_UuidParse(benchmark::State& state) {
    const auto uuid_str = MakeUuidString();

    for (auto _ : state) {
        auto result = Uuid::FromString(uuid_str);
        benchmark::DoNotOptimize(result);
    }
}

FastMutex m;

static void BM_FastMutex(benchmark::State& state) {
    for (auto _ : state) {
        m.lock();
        m.unlock();
    }
}

Spinlock l;

static void BM_Spinlock(benchmark::State& state) {
    for (auto _ : state) {
        l.lock();
        l.unlock();
    }
}

static void BM_SpinLockFreeStack(benchmark::State& state) {
    static SpinLockFreeStack<int32_t> ws_queue(64);

    for (auto _ : state) {
        for (auto i = 0; i < 8; ++i) {
            ws_queue.TryEnqueue(1);
            int32_t result;
            ws_queue.TryDequeue(result);
        }
    }
}

static void BM_LIFOQueue(benchmark::State& state) {
    using Queue = BlockingQueue<int32_t, FastMutex, LIFOQueue<int32_t>>;
    static Queue lifo_queue(64);

    for (auto _ : state) {
        for (auto i = 0; i < 8; ++i) {
            lifo_queue.TryEnqueue(1);
            int32_t result;
            lifo_queue.TryDequeue(result);
        }        
    }
}

static void BM_CircularBuffer(benchmark::State& state) {
    using Queue = BlockingQueue<int32_t>;
    static Queue blocking_queue(64);

    for (auto _ : state) {
        for (auto i = 0; i < 8; ++i) {
            blocking_queue.TryEnqueue(1);
            int32_t result;
            blocking_queue.TryDequeue(result);
        }
    }
}

static void BM_Builtin_Rotl(benchmark::State& state) {
    for (auto _ : state) {
#ifdef XAMP_OS_WIN
        auto result = _rotl64(0x76e15d3efefdcbbf, 7);
#else
        auto result = __builtin_rotateleft64(0x76e15d3efefdcbbf, 7);
#endif
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
}

static void BM_Rotl(benchmark::State& state) {
    for (auto _ : state) {
        auto result = Rotl64(0x76e15d3efefdcbbf, 7);
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
}

//BENCHMARK(BM_InterleavedToPlanarConvertToInt8_AVX)->Range(4096, 8 << 10);
//BENCHMARK(BM_InterleavedToPlanarConvertToInt8)->Range(4096, 8 << 10);

//BENCHMARK(BM_Builtin_Rotl);
//BENCHMARK(BM_Rotl);

//BENCHMARK(BM_FastMutex)->ThreadRange(1, 32);
//BENCHMARK(BM_Spinlock)->ThreadRange(1, 32);

//BENCHMARK(BM_Builtin_UuidParse);
//BENCHMARK(BM_UuidParse);
BENCHMARK(BM_Xoshiro256StarStarRandom);
BENCHMARK(BM_Xoshiro256PlusRandom);
BENCHMARK(BM_Xoshiro256PlusPlusRandom);
BENCHMARK(BM_ChaCha20Random);
//BENCHMARK(BM_default_random_engine);
//BENCHMARK(BM_PRNG);
//BENCHMARK(BM_PRNG_GetInstance);
//BENCHMARK(BM_PRNG_SharedGetInstance);

//BENCHMARK(BM_unordered_set);
//BENCHMARK(BM_FindRobinHoodHashSet);
//BENCHMARK(BM_unordered_map);
//BENCHMARK(BM_FindRobinHoodHashMap);

//BENCHMARK(BM_FowardListSort)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_VectorSort)->RangeMultiplier(2)->Range(4096, 8 << 10);

//BENCHMARK(BM_LruCache_Find)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_LruCache_Add)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_LruCache_AddOrUpdate)->RangeMultiplier(2)->Range(4096, 8 << 10);

//BENCHMARK(BM_FastMemset)->RangeMultiplier(2)->Range(4096, 8 << 16);
//BENCHMARK(BM_StdtMemset)->RangeMultiplier(2)->Range(4096, 8 << 16);
//BENCHMARK(BM_FastMemcpy)->RangeMultiplier(2)->Range(4096, 8 << 16);
//BENCHMARK(BM_StdtMemcpy)->RangeMultiplier(2)->Range(4096, 8 << 16);

//BENCHMARK(BM_ConvertToInt2432Avx)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_ConvertToInt2432)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_InterleavedToPlanarConvertToInt32_AVX)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_InterleavedToPlanarConvertToInt32)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_FFT)->RangeMultiplier(2)->Range(4096, 8 << 12);

//BENCHMARK(BM_SpinLockFreeStack)->ThreadRange(1, 128);
//BENCHMARK(BM_LIFOQueue)->ThreadRange(1, 128);
//BENCHMARK(BM_CircularBuffer)->ThreadRange(1, 128);

//BENCHMARK(BM_async_pool)->RangeMultiplier(2)->Range(8, 8 << 8);
#ifdef XAMP_OS_WIN
//BENCHMARK(BM_std_for_each_par)->RangeMultiplier(2)->Range(8, 8 << 8);
#endif
//BENCHMARK(BM_LeastLoadThreadPool)->RangeMultiplier(2)->Range(8, 8 << 8);
//BENCHMARK(BM_ChildStealPolicyRandomThreadPool)->RangeMultiplier(2)->Range(8, 8 << 8);
//BENCHMARK(BM_ContinuationStealPolicyRandomThreadPool)->RangeMultiplier(2)->Range(8, 8 << 8);

int main(int argc, char** argv) {
    LoggerManager::GetInstance()
        .AddDebugOutput()
        .AddLogFile("xamp.log")
        .Startup();

    // For debug use!
    XAMP_LOG_DEBUG("Logger init success.");

    LoggerManager::GetInstance().GetLogger(kWASAPIThreadPoolLoggerName)
        ->SetLevel(LOG_LEVEL_DEBUG);
    LoggerManager::GetInstance().GetLogger(kPlaybackThreadPoolLoggerName)
        ->SetLevel(LOG_LEVEL_DEBUG);

    LoggerManager::GetInstance().GetLogger("BM_LeastLoadThreadPool")
        ->SetLevel(LOG_LEVEL_DEBUG);
    LoggerManager::GetInstance().GetLogger("BM_RoundRubinThreadPool")
        ->SetLevel(LOG_LEVEL_DEBUG);
    LoggerManager::GetInstance().GetLogger("BM_RandomThreadPool")
        ->SetLevel(LOG_LEVEL_DEBUG);

    LoggerManager::GetInstance().GetLogger("BM_ChildStealPolicyRandomThreadPool")
        ->SetLevel(LOG_LEVEL_DEBUG);
    LoggerManager::GetInstance().GetLogger("BM_ContinuationStealPolicyRandomThreadPool")
        ->SetLevel(LOG_LEVEL_DEBUG);

    XampIniter initer;
    initer.Init();

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return -1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
}
