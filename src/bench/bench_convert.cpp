#include <benchmark/benchmark.h>

#include <vector>
#include <queue>

#include <base/memory.h>
#include <base/rng.h>
#include <base/dataconverter.h>
#include <base/stl.h>
#include <base/circularbuffer.h>
#include <player/fft.h>

using namespace xamp::base;
using namespace xamp::player;

static void BM_ConvertToInt2432SSE(benchmark::State& state) {
    std::vector<int32_t> output(4096);
    std::vector<float> input(4096);

    AudioConvertContext ctx;
    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    for (auto& s : input) {
        s = (float)RNG::Instance()(0.0, 1.0);
    }

    ctx = MakeConvert(input_format, output_format, 2048);

    for (auto _ : state) {
        (void)DataConverter<InterleavedFormat::INTERLEAVED, InterleavedFormat::INTERLEAVED>::
            ConvertToInt2432(output.data(), input.data(), ctx);
    }
}

BENCHMARK(BM_ConvertToInt2432SSE);

static void BM_ConvertToInt2432(benchmark::State& state) {
    std::vector<int32_t> output(4096);
    std::vector<float> input(4096);

    AudioConvertContext ctx;
    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    for (auto& s : input) {
        s = (float)RNG::Instance()(0.0, 1.0);
    }

    ctx = MakeConvert(input_format, output_format, 2048);

    for (auto _ : state) {
        (void)Convert2432Helper(output.data(), input.data(), ctx);
    }
}

BENCHMARK(BM_ConvertToInt2432);

static void BM_FindRobinHoodHashMap(benchmark::State& state) {
    RobinHoodHashMap<int32_t, int32_t> map;
    for (auto _ : state) {
        map.emplace(RNG::Instance().GetInt(), 0);
        (void)map.find(RNG::Instance().GetInt());
    }
}

BENCHMARK(BM_FindRobinHoodHashMap);

static void BM_FindStdHashMap(benchmark::State& state) {
    std::unordered_map<std::int32_t, int32_t> map;
    for (auto _ : state) {
        map.emplace(RNG::Instance().GetInt(), 0);
        (void)map.find(RNG::Instance().GetInt());
    }
}

BENCHMARK(BM_FindStdHashMap);

static void BM_FindRobinHoodHashSet(benchmark::State& state) {
    RobinHoodSet<int32_t> map;
    for (auto _ : state) {
        map.emplace(RNG::Instance().GetInt());
        (void)map.find(RNG::Instance().GetInt());
    }
}

BENCHMARK(BM_FindRobinHoodHashSet);

static void BM_FindStdHashSet(benchmark::State& state) {
    std::unordered_set<std::int32_t> map;
    for (auto _ : state) {
        map.emplace(RNG::Instance().GetInt());
        (void)map.find(RNG::Instance().GetInt());
    }
}

BENCHMARK(BM_FindStdHashSet);

static void BM_StdQueue(benchmark::State& state) {
    std::queue<int32_t> q;
    for (auto _ : state) {
        q.push(42);
        q.pop();
    }
}

BENCHMARK(BM_StdQueue);

static void BM_CircularBuffer(benchmark::State& state) {
    CircularBuffer<int32_t> buffer(10);
    for (auto _ : state) {
        buffer.push(42);
        buffer.pop();
    }
}

BENCHMARK(BM_CircularBuffer);

static void BM_StdCppFFTForward(benchmark::State& state) {
    FFT fft;

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::complex<float>> data(8192);
        for (auto i = 0; i < data.size(); ++i) {
            data[i] = std::cos(2.0 * PI * i / 256.0);
        }
        std::valarray<std::complex<float>> sin_wav(data.data(), data.size());
        state.ResumeTiming();
        fft.Forward(sin_wav);
    }
}

BENCHMARK(BM_StdCppFFTForward);

static void BM_StdCppFFTInverse(benchmark::State& state) {
    FFT fft;    

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::complex<float>> data(8192);
        for (auto i = 0; i < data.size(); ++i) {
            data[i] = std::cos(2.0 * PI * i / 256.0);
        }
        std::valarray<std::complex<float>> sin_wav(data.data(), data.size());
        state.ResumeTiming();
        fft.Inverse(sin_wav);
    }
}

BENCHMARK(BM_StdCppFFTInverse);

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return -1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
}