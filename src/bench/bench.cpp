#include <benchmark/benchmark.h>

#include <base/threadpool.h>
#include <base/logger.h>

#include <atomic>
#include <chrono>
#include <future>
#include <latch>
#include <thread>
#include <vector>

namespace {
    using namespace xamp::base;

    const auto kThreadCount = std::thread::hardware_concurrency();
    constexpr auto kBulkSize = 1U;

    bool isPrime(uint32_t n) {
        if (n <= 1) return false;
        if (n == 2) return true;
        if (n % 2 == 0) return false;

        for (uint32_t i = 3; i <= n / i; i += 2) {
            if (n % i == 0) return false;
        }

        return true;
    }

    uint32_t makePrimeCandidate(size_t task_index, size_t value_index) noexcept {
        constexpr uint32_t kBaseCandidate = 1'000'000'007U;
        return kBaseCandidate
            + static_cast<uint32_t>(task_index * 997U)
            + static_cast<uint32_t>(value_index * 131U);
    }

    std::shared_ptr<IThreadPool> makeBenchPool() {
        return ThreadPoolBuilder::MakeThreadPool("BenchThreadPool",
            kThreadCount,
            kBulkSize,
            ThreadPriority::PRIORITY_NORMAL);
    }

    static void BM_ThreadPool_CreateDestroy(benchmark::State& state) {
        for ([[maybe_unused]] auto _ : state) {
            auto executor = makeBenchPool();
            benchmark::DoNotOptimize(executor.get());
            executor->stop();
        }
    }

    static void BM_ThreadPool_BurstTinyTasks(benchmark::State& state) {
        const auto task_count = static_cast<size_t>(state.range(0));
        auto executor = makeBenchPool();

        for ([[maybe_unused]] auto _ : state) {
            std::atomic<size_t> completed{ 0 };
            std::vector<Future<void>> futures;
            futures.reserve(task_count);

            for (size_t i = 0; i < task_count; ++i) {
                futures.emplace_back(executor->spawn([&completed](const auto&) {
                    completed.fetch_add(1, std::memory_order_relaxed);
                    }));
            }

            for (auto& future : futures) {
                future.wait();
            }

            benchmark::DoNotOptimize(completed.load(std::memory_order_relaxed));
        }

        executor->stop();
        state.SetItemsProcessed(state.iterations() * task_count);
    }

    static void BM_ThreadPool_BurstCpuTasks(benchmark::State& state) {
        const auto task_count = static_cast<size_t>(state.range(0));
        const auto work_size = static_cast<size_t>(state.range(1));
        auto executor = makeBenchPool();

        for ([[maybe_unused]] auto _ : state) {
            std::atomic<uint64_t> prime_count{ 0 };
            std::vector<Future<void>> futures;
            futures.reserve(task_count);

            for (size_t task_index = 0; task_index < task_count; ++task_index) {
                futures.emplace_back(executor->spawn([task_index, work_size, &prime_count](const auto&) {
                    uint64_t local_prime_count = 0;
                    for (size_t i = 0; i < work_size; ++i) {
                        local_prime_count += isPrime(makePrimeCandidate(task_index, i)) ? 1U : 0U;
                    }
                    prime_count.fetch_add(local_prime_count, std::memory_order_relaxed);
                    }));
            }

            for (auto& future : futures) {
                future.wait();
            }

            benchmark::DoNotOptimize(prime_count.load(std::memory_order_relaxed));
        }

        executor->stop();
        state.SetItemsProcessed(state.iterations() * task_count);
    }

    static void BM_StdAsync_BurstTinyTasks(benchmark::State& state) {
        const auto task_count = static_cast<size_t>(state.range(0));

        for ([[maybe_unused]] auto _ : state) {
            std::atomic<size_t> completed{ 0 };
            std::vector<std::future<void>> futures;
            futures.reserve(task_count);

            for (size_t i = 0; i < task_count; ++i) {
                futures.emplace_back(std::async(std::launch::async, [&completed] {
                    completed.fetch_add(1, std::memory_order_relaxed);
                    }));
            }

            for (auto& future : futures) {
                future.wait();
            }

            benchmark::DoNotOptimize(completed.load(std::memory_order_relaxed));
        }

        state.SetItemsProcessed(state.iterations() * task_count);
    }

    static void BM_StdAsync_BurstCpuTasks(benchmark::State& state) {
        const auto task_count = static_cast<size_t>(state.range(0));
        const auto work_size = static_cast<size_t>(state.range(1));

        for ([[maybe_unused]] auto _ : state) {
            std::atomic<uint64_t> prime_count{ 0 };
            std::vector<std::future<void>> futures;
            futures.reserve(task_count);

            for (size_t task_index = 0; task_index < task_count; ++task_index) {
                futures.emplace_back(std::async(std::launch::async, [task_index, work_size, &prime_count] {
                    uint64_t local_prime_count = 0;
                    for (size_t i = 0; i < work_size; ++i) {
                        local_prime_count += isPrime(makePrimeCandidate(task_index, i)) ? 1U : 0U;
                    }
                    prime_count.fetch_add(local_prime_count, std::memory_order_relaxed);
                    }));
            }

            for (auto& future : futures) {
                future.wait();
            }

            benchmark::DoNotOptimize(prime_count.load(std::memory_order_relaxed));
        }

        state.SetItemsProcessed(state.iterations() * task_count);
    }

    static void BM_ThreadPool_IdleWake(benchmark::State& state) {
        auto executor = makeBenchPool();

        for ([[maybe_unused]] auto _ : state) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));

            std::latch done{ 1 };
            const auto begin = std::chrono::steady_clock::now();

            auto future = executor->spawn([&done](const auto&) {
                done.count_down();
                });
            done.wait();
            future.wait();

            const auto elapsed = std::chrono::steady_clock::now() - begin;
            state.SetIterationTime(std::chrono::duration<double>(elapsed).count());
        }

        executor->stop();
    }

    static void BM_StdAsync_Wake(benchmark::State& state) {
        for ([[maybe_unused]] auto _ : state) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));

            std::latch done{ 1 };
            const auto begin = std::chrono::steady_clock::now();

            auto future = std::async(std::launch::async, [&done] {
                done.count_down();
                });
            done.wait();
            future.wait();

            const auto elapsed = std::chrono::steady_clock::now() - begin;
            state.SetIterationTime(std::chrono::duration<double>(elapsed).count());
        }
    }

    BENCHMARK(BM_ThreadPool_CreateDestroy);
    BENCHMARK(BM_ThreadPool_BurstTinyTasks)
        ->Arg(64)
        ->Arg(256)
        ->Arg(1024)
        ->Arg(4096);
    BENCHMARK(BM_ThreadPool_BurstCpuTasks)
        ->Args({ 64, 1024 })
        ->Args({ 256, 1024 })
        ->Args({ 1024, 1024 });
    BENCHMARK(BM_ThreadPool_IdleWake)
        ->Iterations(256)
        ->UseManualTime();
    BENCHMARK(BM_StdAsync_BurstTinyTasks)
        ->Arg(64)
        ->Arg(256)
        ->Arg(1024);
    BENCHMARK(BM_StdAsync_BurstCpuTasks)
        ->Args({ 64, 1024 })
        ->Args({ 256, 1024 })
        ->Args({ 1024, 1024 });
    BENCHMARK(BM_StdAsync_Wake)
        ->Iterations(256)
        ->UseManualTime();
}

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);

    XampLoggerFactory
        .addDebugOutput()
        .startup();

    XAMP_LOG_DEBUG("Logger init success.");

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return -1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
}
