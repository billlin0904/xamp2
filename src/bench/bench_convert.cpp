#include <benchmark/benchmark.h>

#include <iostream>
#include <vector>
#include <queue>

#include <base/scopeguard.h>
#include <base/audiobuffer.h>
#include <base/threadpool.h>
#include <base/memory.h>
#include <base/rng.h>
#include <base/fastmutex.h>
#include <base/dataconverter.h>
#include <base/stl.h>

using namespace xamp::base;

static std::vector<float> GetRandomSamples() {
    std::vector<float> input(1024 * 1024);
    for (auto& s : input) {
        s = RNG::GetInstance()(-1.0F, 1.0F);
    }
    return input;
}

#if 0
static void BM_FutexQueue(benchmark::State& state) {
    BoundedQueue<int32_t, FastMutex, FutexMutexConditionVariable> Q(1024 * 1024 * 1024);
    std::atomic<bool> running(true);

    // producer

    auto p1 = std::async(std::launch::async, [&running, &Q]() {
        while (running) {
            Q.Enqueue(42);
        }
        });
    auto p2 = std::async(std::launch::async, [&running, &Q]() {
        while (running) {
            Q.Enqueue(42);
        }
        });
    auto p3 = std::async(std::launch::async, [&running, &Q]() {
        while (running) {
            Q.Enqueue(42);
        }
        });
    auto p4 = std::async(std::launch::async, [&running, &Q]() {
        while (running) {
            Q.Enqueue(42);
        }
        });


    for (auto _ : state) {
        // consumer        
        auto c1 = std::async(std::launch::async, [&Q]() {
            int32_t value = 0;
            Q.Dequeue(value);
            });
        auto c2 = std::async(std::launch::async, [&Q]() {
            int32_t value = 0;
            Q.Dequeue(value);
            });
        auto c3 = std::async(std::launch::async, [&Q]() {
            int32_t value = 0;
            Q.Dequeue(value);
            });
        auto c4 = std::async(std::launch::async, [&Q]() {
            int32_t value = 0;
            Q.Dequeue(value);
            });
        c1.get();
        c2.get();
        c3.get();
        c4.get();
    }

    running = false;

    p1.get();
    p2.get();
    p3.get();
    p4.get();
}

BENCHMARK(BM_FutexQueue);

static void BM_MutexQueue(benchmark::State& state) {
    BoundedQueue<int32_t> Q(1024 * 1024 * 1024);
    std::atomic<bool> running(true);

    // producer

    auto p1 = std::async(std::launch::async, [&running, &Q]() {
        while (running) {
            Q.Enqueue(42);
        }
        });
    auto p2 = std::async(std::launch::async, [&running, &Q]() {
        while (running) {
            Q.Enqueue(42);
        }
        });
    auto p3 = std::async(std::launch::async, [&running, &Q]() {
        while (running) {
            Q.Enqueue(42);
        }
        });
    auto p4 = std::async(std::launch::async, [&running, &Q]() {
        while (running) {
            Q.Enqueue(42);
        }
        });


    for (auto _ : state) {
        // consumer        
        auto c1 = std::async(std::launch::async, [&Q]() {
            int32_t value = 0;
            Q.Dequeue(value);
            });
        auto c2 = std::async(std::launch::async, [&Q]() {
            int32_t value = 0;
            Q.Dequeue(value);
            });
        auto c3 = std::async(std::launch::async, [&Q]() {
            int32_t value = 0;
            Q.Dequeue(value);
            });
        auto c4 = std::async(std::launch::async, [&Q]() {
            int32_t value = 0;
            Q.Dequeue(value);
            });
        c1.get();
        c2.get();
        c3.get();
        c4.get();
    }

    running = false;

    p1.get();
    p2.get();
    p3.get();
    p4.get();
}

BENCHMARK(BM_MutexQueue);
#endif

#if 0
static void BM_StdRandom(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0);
    for (auto _ : state) {
        dis(gen);
    }
}

BENCHMARK(BM_StdRandom);

static void BM_XoroshiroRandom(benchmark::State& state) {
    RNG::GetInstance();
    for (auto _ : state) {
        RNG::GetInstance().GetInt();
    }
}

BENCHMARK(BM_XoroshiroRandom);

static void BM_AudioBuffer(benchmark::State& state) {
    AudioBuffer<float> buffer(4096 * 10);
    std::atomic<bool> running(true);

    auto push_task = ThreadPool::GetInstance().Spawn([&buffer, &running]()
        {
            float s[1024] = {0};
            while (running)
            {
                buffer.TryWrite(s, 1024);
            }
        });

    float r[128] = { 0 };
    for (auto _ : state) {
        buffer.TryRead(r, 128);
    }

    running = false;
    push_task.get();
}

BENCHMARK(BM_AudioBuffer);

static void BM_LockAudioBuffer(benchmark::State& state) {
    FastMutex mutex;
    float buffer[4096 * 10];
    std::atomic<bool> running(true);

    auto push_task = ThreadPool::GetInstance().Spawn([&buffer, &running, &mutex]()
        {
            float s[1024] = { 0 };
            while (running)
            {
                std::lock_guard<FastMutex> lock_guard{mutex};
                MemoryCopy(buffer, s, 1024);
            }
        });

    float r[128] = { 0 };
    for (auto _ : state) {
        std::lock_guard<FastMutex> lock_guard{ mutex };
        MemoryCopy(r, buffer, 128);
    }

    running = false;
    push_task.get();
}

BENCHMARK(BM_LockAudioBuffer);

static void BM_ClampSampleSSE2(benchmark::State& state) {
    auto input = GetRandomSamples();
    for (auto _ : state) {
        for (auto &v : input) {
            v = ClampSampleSSE2(v);
        }
    }
}

BENCHMARK(BM_ClampSampleSSE2);

static void BM_ClampSample(benchmark::State& state) {
    auto input = GetRandomSamples();
    for (auto _ : state) {
        ClampSample(input.data(), input.size());
    }
}

BENCHMARK(BM_ClampSample);
#endif

#if 1
static void BM_ThreadPool(benchmark::State& state) {
    ThreadPool thread_pool;
    for (auto _ : state) {
        for (auto i =0 ; i < 10000; ++i) {
            thread_pool.Spawn([]() {}).get();
        }        
    }
}

BENCHMARK(BM_ThreadPool);

static void BM_StdThreadPool(benchmark::State& state) {
    for (auto _ : state) {        
        for (auto i = 0; i < 10000; ++i) {
            std::async(std::launch::async, []() {}).get();
        }
    }
}

BENCHMARK(BM_StdThreadPool);
#endif

#if 0
static void BM_ConvertToInt2432SSE(benchmark::State& state) {
    std::vector<int32_t> output(4096);
    std::vector<float> input(4096);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    for (auto& s : input) {
        s = RNG::GetInstance()(0.0F, 1.0F);
    }

    auto ctx = MakeConvert(input_format, output_format, 2048);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            ConvertToInt2432(output.data(), input.data(), ctx);
    }
}

BENCHMARK(BM_ConvertToInt2432SSE);

static void BM_ConvertToInt2432(benchmark::State& state) {
    std::vector<int32_t> output(4096);
    std::vector<float> input(4096);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    for (auto& s : input) {
        s = RNG::GetInstance()(0.0F, 1.0F);
    }

    auto ctx = MakeConvert(input_format, output_format, 2048);

    for (auto _ : state) {
        Convert2432Helper(output.data(), input.data(), ctx);
    }
}

BENCHMARK(BM_ConvertToInt2432);

static void BM_FindRobinHoodHashMap(benchmark::State& state) {
    HashMap<int32_t, int32_t> map;
    for (auto _ : state) {
        map.emplace(RNG::GetInstance().GetInt(), 0);
        (void)map.find(RNG::GetInstance().GetInt());
    }
}

BENCHMARK(BM_FindRobinHoodHashMap);

static void BM_FindStdHashMap(benchmark::State& state) {
    std::unordered_map<std::int32_t, int32_t> map;
    for (auto _ : state) {
        map.emplace(RNG::GetInstance().GetInt(), 0);
        (void)map.find(RNG::GetInstance().GetInt());
    }
}

BENCHMARK(BM_FindStdHashMap);

static void BM_FindRobinHoodHashSet(benchmark::State& state) {
    HashSet<int32_t> map;
    for (auto _ : state) {
        map.emplace(RNG::GetInstance().GetInt());
        (void)map.find(RNG::GetInstance().GetInt());
    }
}

BENCHMARK(BM_FindRobinHoodHashSet);

static void BM_FindStdHashSet(benchmark::State& state) {
    std::unordered_set<std::int32_t> map;
    for (auto _ : state) {
        map.emplace(RNG::GetInstance().GetInt());
        (void)map.find(RNG::GetInstance().GetInt());
    }
}

BENCHMARK(BM_FindStdHashSet);

static void BM_FastMemcpy(benchmark::State& state) {
    std::vector<char> src(kCacheAlignSize * 1024 * 1024);
    std::vector<char> dest(kCacheAlignSize * 1024 * 1024);

    for (auto _ : state) {
        MemoryCopy(dest.data(), src.data(), src.size());
    }
}

BENCHMARK(BM_FastMemcpy);

static void BM_StdtMemcpy(benchmark::State& state) {
    std::vector<char> src(kCacheAlignSize * 1024 * 1024);
    std::vector<char> dest(kCacheAlignSize * 1024 * 1024);

    for (auto _ : state) {
        std::memcpy(dest.data(), src.data(), src.size());
    }
}

BENCHMARK(BM_StdtMemcpy);
#endif

#if 0
static void BM_ConvertToInt32(benchmark::State& state) {
    std::vector<int32_t> output(4096);
    std::vector<float> input(4096);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    for (auto& s : input) {
        s = RNG::GetInstance()(0.0F, 1.0F);
    }

    std::cin.get();

    auto ctx = MakeConvert(input_format, output_format, 2048);

    for (auto _ : state) {
        ConvertHelper<int32_t>(output.data(), input.data(), kFloat32Scale, ctx);
    }
}

BENCHMARK(BM_ConvertToInt32);


static void BM_PackedFormatConvertToInt32(benchmark::State& state) {
    std::vector<int32_t> output(4096);
    std::vector<float> input(4096);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    for (auto& s : input) {
        s = RNG::GetInstance()(0.0F, 1.0F);
    }

    const auto ctx = MakeConvert(input_format, output_format, 2048);

    for (auto _ : state) {
        DataConverter<PackedFormat::PLANAR,
            PackedFormat::INTERLEAVED>::Convert(
                output.data(),
                input.data(),
                ctx);
    }
}

BENCHMARK(BM_PackedFormatConvertToInt32);
#endif

#if 0
static void BM_TestDsdFileFormat(benchmark::State& state) {
    for (auto _ : state) {
        xamp::player::TestDsdFileFormat(L"C:\\Users\\rdbill0452\\Music\\test.dff");
    }
}

BENCHMARK(BM_TestDsdFileFormat);

static void BM_TestDsdFileFormatStd(benchmark::State& state) {
    for (auto _ : state) {
        xamp::player::TestDsdFileFormatStd(L"C:\\Users\\rdbill0452\\Music\\test.dff");
    }
}

BENCHMARK(BM_TestDsdFileFormatStd);
#endif

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

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return -1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
    std::cin.get();
}