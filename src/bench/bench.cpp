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
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/executor.h>
#include <base/rcu_ptr.h>
#include <base/uuidof.h>
#include <base/google_siphash.h>
#include <base/blake3hash.h>
#include <base/sfc64.h>

#include <stream/fft.h>

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

using namespace xamp::player;
using namespace xamp::base;
using namespace xamp::stream;

XAMP_DECLARE_LOG_NAME(BM_RcuPtr);
XAMP_DECLARE_LOG_NAME(BM_LeastLoadPolicyThreadPool);
XAMP_DECLARE_LOG_NAME(BM_RobinStealPolicyThreadPool);
XAMP_DECLARE_LOG_NAME(BM_RandomPolicyThreadPool);

static void BM_RcuPtr(benchmark::State& state) {
    const auto thread_pool = MakeThreadPoolExecutor(kBM_RcuPtrLoggerName);
    LoggerManager::GetInstance().GetLogger(kBM_RcuPtrLoggerName)
        ->SetLevel(LOG_LEVEL_OFF);

    const auto length = state.range(0);
    RcuPtr<int> total(std::make_shared<int>());

    for (auto _ : state) {
	    Executor::ParallelFor(*thread_pool, 0, length, [&total](auto item) {
			total.copy_update([item](auto cp) {
				*cp += item;
				});
        }, 16);
    }
}

static void BM_RcuPtrMutex(benchmark::State& state) {
    const auto thread_pool = MakeThreadPoolExecutor(kBM_RcuPtrLoggerName);
    LoggerManager::GetInstance().GetLogger(kBM_RcuPtrLoggerName)
        ->SetLevel(LOG_LEVEL_OFF);

    const auto length = state.range(0);
    int total = 0;
    FastMutex mutex;

    for (auto _ : state) {
	    Executor::ParallelFor(*thread_pool, 0, length, [&total, &mutex](auto item) {
            std::lock_guard<FastMutex> guard{ mutex };
			total += item;
            }, 16);
    }
}

static void BM_LeastLoadPolicyThreadPool(benchmark::State& state) {
    const auto thread_pool = MakeThreadPoolExecutor(
        kBM_LeastLoadPolicyThreadPoolLoggerName);

    LoggerManager::GetInstance().GetLogger(kBM_LeastLoadPolicyThreadPoolLoggerName)
        ->SetLevel(LOG_LEVEL_OFF);

    const auto length = state.range(0);
    std::vector<int> n(length);
    std::iota(n.begin(), n.end(), 1);
    std::atomic<int64_t> total;
    for (auto _ : state) {
	    Executor::ParallelFor(*thread_pool, n, [&total](auto item) {
            total += item;
            }, std::chrono::milliseconds(16));
    }
}

static void BM_ThreadLocalRandomPolicyThreadPool(benchmark::State& state) {
    const auto thread_pool = MakeThreadPoolExecutor(
        kBM_RandomPolicyThreadPoolLoggerName,
        TaskSchedulerPolicy::ROUND_ROBIN_POLICY);

    LoggerManager::GetInstance().GetLogger(kBM_RandomPolicyThreadPoolLoggerName)
        ->SetLevel(LOG_LEVEL_OFF);

    const auto length = state.range(0);
    std::vector<int> n(length);
    std::iota(n.begin(), n.end(), 1);
    std::atomic<int64_t> total;
    for (auto _ : state) {
        Executor::ParallelFor(*thread_pool, n, [&total](auto item) {
            total += item;
            }, std::chrono::milliseconds(16));
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
        StdAsyncParallelFor(n, [&total](auto item) {
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
        std::for_each(std::execution::par,
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

static void BM_default_random_engine(benchmark::State& state) {
    std::random_device rd;
    std::default_random_engine engine(rd());
    for (auto _ : state) {
        size_t n = std::uniform_int_distribution<int32_t>(INT32_MIN, INT32_MAX)(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(sizeof(int64_t) * static_cast<int64_t>(state.iterations()));
}

static void BM_Sfc64Random(benchmark::State& state) {
	Sfc64Engine<> engine;
    for (auto _ : state) {
        size_t n = std::uniform_int_distribution<int32_t>(INT32_MIN, INT32_MAX)(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(sizeof(int64_t) * static_cast<int64_t>(state.iterations()));
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
    state.SetBytesProcessed(sizeof(int64_t) * static_cast<int64_t>(state.iterations()));
}

static void BM_PRNG_SharedGetInstance(benchmark::State& state) {
    for (auto _ : state) {
        size_t n = SharedSingleton<PRNG>::GetInstance().NextInt64();
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(sizeof(int64_t) * static_cast<int64_t>(state.iterations()));
}

static void BM_ConvertToIntAvx(benchmark::State& state) {
    auto length = state.range(0);

    auto output = Vector<int32_t>(length);
    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            Convert(output.data(), input.data(), ctx);
    }

    state.SetBytesProcessed(length * static_cast<int64_t>(state.iterations()) * sizeof(float));
}

static void BM_ConvertToShortAvx(benchmark::State& state) {
    auto length = state.range(0);

    auto output = Vector<int16_t>(length);
    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            Convert(output.data(), input.data(), ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_ConvertToInt2432Avx(benchmark::State& state) {
    auto length = state.range(0);

    auto output = Vector<int32_t>(length);
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

#ifdef XAMP_USE_BENCHMAKR
static void BM_ConvertToInt2432(benchmark::State& state) {
    auto length = state.range(0);

    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);
    auto output = Vector<int32_t>(length);

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

static void BM_ConvertToInt(benchmark::State& state) {
    auto length = state.range(0);

    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);
    auto output = Vector<int32_t>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            ConvertInt32Bench(output.data(), input.data(), ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_ConvertToShort(benchmark::State& state) {
    auto length = state.range(0);

    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);
    auto output = Vector<int16_t>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            ConvertInt16Bench(output.data(), input.data(), ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}
#endif

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

    /*auto input = Singleton<PRNG>::GetInstance().NextBytes(length);
    auto output = Vector<int8_t>(length * 2);*/

    const Vector<int8_t> input{
    	0, 1, 0, 2, 0, 3, 0, 4,
        0, 5, 0, 6, 0, 7, 0, 8,
        0, 9, 0, 10, 0, 11, 0, 12,
        0, 13, 0, 14, 0, 15, 0, 16,
        0, 17, 0, 18, 0, 19, 0, 20,
        0, 21, 0, 22, 0, 23, 0, 24,
        0, 25, 0, 26, 0, 27, 0, 28,
        0, 29, 0, 30, 0, 31, 0, 32,
    };

    Vector<int8_t> output(input.size());

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    input_format.SetPackedFormat(PackedFormat::INTERLEAVED);
    output_format.SetPackedFormat(PackedFormat::PLANAR);

    for (auto _ : state) {
        InterleaveToPlanar<int8_t, int8_t>::Convert(input.data(),
            output.data(),
            output.data() + 32,
            32);
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

    input_format.SetPackedFormat(PackedFormat::INTERLEAVED);
    output_format.SetPackedFormat(PackedFormat::PLANAR);

    const auto ctx = MakeConvert(input_format, output_format, length / 2);


    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::PLANAR>::Convert(
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
    ForwardList<TrackInfo> list;

    for (auto i = 0; i < length; ++i) {
        TrackInfo metadata;
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
    std::vector<TrackInfo> list;

    for (auto i = 0; i < length; ++i) {
        TrackInfo metadata;
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

static void BM_LruCache_GetOrAdd(benchmark::State& state) {
    auto length = state.range(0);

    LruCache<int64_t, int64_t> cache(length / 2);

    for (auto i = 0; i < length; ++i) {
        cache.Add(
            SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000),\
            SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000));
    }
    
    for (auto _ : state) {
        state.PauseTiming();
        auto key = SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000);
        state.ResumeTiming();
        cache.GetOrAdd(key, []() {
            return SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000);
        });
    }
}

static void BM_LruCache_Add(benchmark::State& state) {
    auto length = state.range(0);

    LruCache<int64_t, int64_t> cache(length / 2);

    for (auto _ : state) {
        state.PauseTiming();
        auto key = SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000);
        auto value = SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000);
        state.ResumeTiming();
        cache.Add(key, value);
    }
}

static void BM_LruCache_AddOrUpdate(benchmark::State& state) {
    auto length = state.range(0);

    LruCache<int64_t, int64_t> cache(length / 2);

    for (auto _ : state) {
        state.PauseTiming();
        auto key = SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000);
        auto value = SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000);
        state.ResumeTiming();
        cache.AddOrUpdate(key, value);
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

static void BM_UuidCompilerTime(benchmark::State& state) {
    using namespace UuidLiterals;

    for (auto _ : state) {
        auto result = "4293818B-D5E9-4FBE-BA14-B15A68F1DEEA"_uuid;
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

static void BM_BlockingQueue(benchmark::State& state) {
}

static void BM_SpmcQueue(benchmark::State& state) {
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

static void BM_Blake3Hash(benchmark::State& state) {
    auto length = state.range(0);
    for (auto _ : state) {
        state.PauseTiming();
        auto random_string = Singleton<PRNG>::GetInstance().GetRandomString(length);
        Blake3Hash hasher;
        state.ResumeTiming();
        hasher.Update(random_string);
        auto result = hasher.Get64bitHash();
        benchmark::DoNotOptimize(result);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(int64_t));
}

static void BM_GoogleSipHash(benchmark::State& state) {
    auto length = state.range(0);
    for (auto _ : state) {
        state.PauseTiming();
        auto random_string = Singleton<PRNG>::GetInstance().GetRandomString(length);
        state.ResumeTiming();
        GoogleSipHash<> hasher;
        hasher.Update(random_string);
        auto result = hasher.GetHash();
        benchmark::DoNotOptimize(result);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(int64_t));
}

//BENCHMARK(BM_InterleavedToPlanarConvertToInt8_AVX)->Range(4096, 8 << 10);
//BENCHMARK(BM_InterleavedToPlanarConvertToInt8)->Range(4096, 8 << 10);

//BENCHMARK(BM_Builtin_Rotl);
//BENCHMARK(BM_Rotl);

//BENCHMARK(BM_FastMutex)->ThreadRange(1, 32);
//BENCHMARK(BM_Spinlock)->ThreadRange(1, 32);

//BENCHMARK(BM_Builtin_UuidParse);
//BENCHMARK(BM_UuidParse);
//BENCHMARK(BM_UuidCompilerTime);

//BENCHMARK(BM_Sfc64Random);
//BENCHMARK(BM_default_random_engine);

BENCHMARK(BM_Blake3Hash)->RangeMultiplier(2)->Range(128, 8 << 6);
BENCHMARK(BM_GoogleSipHash)->RangeMultiplier(2)->Range(128, 8 << 6);

//BENCHMARK(BM_PRNG);
//BENCHMARK(BM_PRNG_GetInstance);
//BENCHMARK(BM_PRNG_SharedGetInstance);

//BENCHMARK(BM_unordered_set);
//BENCHMARK(BM_FindRobinHoodHashSet);
//BENCHMARK(BM_unordered_map);
//BENCHMARK(BM_FindRobinHoodHashMap);

//BENCHMARK(BM_FowardListSort)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_VectorSort)->RangeMultiplier(2)->Range(4096, 8 << 10);

//BENCHMARK(BM_LruCache_GetOrAdd)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_LruCache_Add)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_LruCache_AddOrUpdate)->RangeMultiplier(2)->Range(4096, 8 << 10);

//BENCHMARK(BM_FastMemset)->RangeMultiplier(2)->Range(4096, 8 << 16);
//BENCHMARK(BM_StdtMemset)->RangeMultiplier(2)->Range(4096, 8 << 16);
//BENCHMARK(BM_FastMemcpy)->RangeMultiplier(2)->Range(4096, 8 << 16);
//BENCHMARK(BM_StdtMemcpy)->RangeMultiplier(2)->Range(4096, 8 << 16);

//BENCHMARK(BM_ConvertToInt2432Avx)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_ConvertToInt2432)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_ConvertToIntAvx)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_ConvertToInt)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_ConvertToShortAvx)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_ConvertToShort)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_InterleavedToPlanarConvertToInt32_AVX)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_InterleavedToPlanarConvertToInt32)->RangeMultiplier(2)->Range(4096, 8 << 12);

//BENCHMARK(BM_FFT)->RangeMultiplier(2)->Range(4096, 8 << 12);

//BENCHMARK(BM_SpinLockFreeStack)->ThreadRange(1, 128);
//BENCHMARK(BM_LIFOQueue)->ThreadRange(1, 128);
//BENCHMARK(BM_CircularBuffer)->ThreadRange(1, 128);

//BENCHMARK(BM_async_pool)->RangeMultiplier(2)->Range(8, 8 << 8);
//#ifdef XAMP_OS_WIN
//BENCHMARK(BM_std_for_each_par)->RangeMultiplier(2)->Range(8, 8 << 8);
//#endif

//BENCHMARK(BM_RandomPolicyThreadPool)->RangeMultiplier(2)->Range(8, 8 << 8);
//BENCHMARK(BM_ThreadLocalRandomPolicyThreadPool)->RangeMultiplier(2)->Range(8, 8 << 8);

//BENCHMARK(BM_RobinStealPolicyThreadPool)->RangeMultiplier(2)->Range(8, 8 << 8);
//BENCHMARK(BM_LeastLoadPolicyThreadPool)->RangeMultiplier(2)->Range(8, 8 << 8);

//BENCHMARK(BM_RcuPtr)->RangeMultiplier(2)->Range(8, 8 << 8);
//BENCHMARK(BM_RcuPtrMutex)->RangeMultiplier(2)->Range(8, 8 << 8);

int main(int argc, char** argv) {
    LoggerManager::GetInstance()
        .AddDebugOutput()
        .Startup();

    // For debug use!
    XAMP_LOG_DEBUG("Logger init success.");

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return -1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
}
