#ifndef _DEBUG
#include <benchmark/benchmark.h>
#include <base/singleton.h>

#include <vector>
#include <queue>

#include <base/memory.h>
#include <base/rng.h>
#include <base/dataconverter.h>
#include <base/stl.h>
#include <player/audio_util.h>

using namespace xamp::base;

static std::vector<float> GetRandomSamples() {
    std::vector<float> input(kCacheAlignSize * 1024 * 1024);
    for (auto& s : input) {
        s = RNG::GetInstance()(-1.0F, 1.0F);
    }
    return input;
}

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
                reinterpret_cast<int32_t*>(output.data()),
                reinterpret_cast<const float*>(input.data()),
                ctx);
    }
}

BENCHMARK(BM_PackedFormatConvertToInt32);

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
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return -1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
}
#endif