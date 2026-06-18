#include <benchmark/benchmark.h>

#include <base/threadpool.h>
#include <base/threadpoolbuilder.h>
#include <base/logger.h>

#include <atomic>
#include <chrono>
#include <future>
#include <latch>
#include <memory>
#include <array>
#include <thread>
#include <vector>

namespace {
    using namespace xamp::base;

    const auto kThreadCount = std::thread::hardware_concurrency();
    constexpr size_t kBenchBulkSize = 2;
    constexpr std::array<size_t, 4> kTinyTaskCounts{ 64, 256, 1024, 4096 };
    constexpr std::array<size_t, 3> kCpuTaskCounts{ 64, 256, 1024 };
    constexpr std::array<size_t, 3> kNestedTaskCounts{ 4, 8, 8 };
    constexpr std::array<size_t, 3> kNestedInnerTaskCounts{ 4, 4, 8 };

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
        return ThreadPoolBuilder::makeThreadPool("BenchThreadPool",
            kThreadCount,
            kBenchBulkSize,
            ThreadPriority::PRIORITY_NORMAL);
    }

    bool waitFuture(Future<void>& future, std::chrono::seconds timeout) {
        return future.wait_for(timeout) == std::future_status::ready;
    }

    static void BM_ThreadPool_CreateDestroy(benchmark::State& state) {
        for ([[maybe_unused]] auto _ : state) {
            auto bench_pool = makeBenchPool();
            benchmark::DoNotOptimize(bench_pool.get());
            bench_pool->stop();
        }
    }

    static void BM_ThreadPool_PostBurstTinyTasks(benchmark::State& state) {
        const auto task_count = static_cast<size_t>(state.range(0));
        auto bench_pool = makeBenchPool();

        for ([[maybe_unused]] auto _ : state) {
            std::atomic<size_t> completed{ 0 };
            std::latch done{ static_cast<ptrdiff_t>(task_count) };

            for (size_t i = 0; i < task_count; ++i) {
                bench_pool->post(
                    ExecuteFlags::EXECUTE_NORMAL,
                    [&completed, &done](const auto&) {
                    completed.fetch_add(1, std::memory_order_relaxed);
                    done.count_down();
                    });
            }

            done.wait();

            benchmark::DoNotOptimize(completed.load(std::memory_order_relaxed));
        }

        bench_pool->stop();
        state.SetItemsProcessed(state.iterations() * task_count);
    }

    static void BM_ThreadPool_SpawnBurstTinyTasks(benchmark::State& state) {
        const auto task_count = static_cast<size_t>(state.range(0));
        auto bench_pool = makeBenchPool();

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

        for ([[maybe_unused]] auto _ : state) {
            std::atomic<size_t> completed{ 0 };
            std::vector<Future<void>> futures;
            futures.reserve(task_count);

            for (size_t i = 0; i < task_count; ++i) {
                futures.emplace_back(bench_pool->spawn(
                    SubmitPolicy::SUBMIT_POLICY_NORMAL,
                    ExecuteFlags::EXECUTE_NORMAL,
                    [&completed](const auto&) {
                    completed.fetch_add(1, std::memory_order_relaxed);
                    }));
            }

            for (auto& future : futures) {
                future.wait();
            }

            benchmark::DoNotOptimize(completed.load(std::memory_order_relaxed));
        }

        bench_pool->stop();
        state.SetItemsProcessed(state.iterations() * task_count);
    }

    static void BM_ThreadPool_PostBurstCpuTasks(benchmark::State& state) {
        const auto task_count = static_cast<size_t>(state.range(0));
        const auto work_size = static_cast<size_t>(state.range(1));
        auto bench_pool = makeBenchPool();

        for ([[maybe_unused]] auto _ : state) {
            std::atomic<uint64_t> prime_count{ 0 };
            std::latch done{ static_cast<ptrdiff_t>(task_count) };

            for (size_t task_index = 0; task_index < task_count; ++task_index) {
                bench_pool->post(
                    ExecuteFlags::EXECUTE_NORMAL,
                    [task_index, work_size, &prime_count, &done](const auto&) {
                    uint64_t local_prime_count = 0;
                    for (size_t i = 0; i < work_size; ++i) {
                        local_prime_count += isPrime(makePrimeCandidate(task_index, i)) ? 1U : 0U;
                    }
                    prime_count.fetch_add(local_prime_count, std::memory_order_relaxed);
                    done.count_down();
                    });
            }

            done.wait();

            benchmark::DoNotOptimize(prime_count.load(std::memory_order_relaxed));
        }

        bench_pool->stop();
        state.SetItemsProcessed(state.iterations() * task_count);
    }

    static void BM_ThreadPool_SpawnBurstCpuTasks(benchmark::State& state) {
        const auto task_count = static_cast<size_t>(state.range(0));
        const auto work_size = static_cast<size_t>(state.range(1));
        auto bench_pool = makeBenchPool();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        for ([[maybe_unused]] auto _ : state) {
            std::atomic<uint64_t> prime_count{ 0 };
            std::vector<Future<void>> futures;
            futures.reserve(task_count);

            for (size_t task_index = 0; task_index < task_count; ++task_index) {
                futures.emplace_back(bench_pool->spawn(
					SubmitPolicy::SUBMIT_POLICY_NORMAL,
                    ExecuteFlags::EXECUTE_NORMAL,
                    [task_index, work_size, &prime_count](const auto&) {
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

        bench_pool->stop();
        state.SetItemsProcessed(state.iterations() * task_count);
    }

    static void BM_ThreadPool_NestedPost(benchmark::State& state) {
        const auto outer_task_count = static_cast<size_t>(state.range(0));
        const auto inner_task_count = static_cast<size_t>(state.range(1));
        auto bench_pool = makeBenchPool();

        for ([[maybe_unused]] auto _ : state) {
            std::atomic<uint64_t> prime_count{ 0 };
            std::latch outer_done{ static_cast<ptrdiff_t>(outer_task_count) };
            std::latch inner_done{ static_cast<ptrdiff_t>(outer_task_count * inner_task_count) };

            for (size_t outer_index = 0; outer_index < outer_task_count; ++outer_index) {
                bench_pool->post(ExecuteFlags::EXECUTE_NORMAL,
                    [bench_pool, outer_index, inner_task_count, &prime_count, &outer_done, &inner_done](const auto&) {
                        for (size_t inner_index = 0; inner_index < inner_task_count; ++inner_index) {
                            bench_pool->post(
                                ExecuteFlags::EXECUTE_NORMAL,
                                [outer_index, inner_index, &prime_count, &inner_done](const auto&) {
                                    const auto candidate = makePrimeCandidate(outer_index, inner_index);
                                    if (isPrime(candidate)) {
                                        prime_count.fetch_add(1, std::memory_order_relaxed);
                                    }
                                    inner_done.count_down();
                                });
                        }

                        outer_done.count_down();
                    });
            }

            outer_done.wait();
            inner_done.wait();
            benchmark::DoNotOptimize(prime_count.load(std::memory_order_relaxed));
        }

        bench_pool->stop();
        state.SetItemsProcessed(state.iterations() * outer_task_count * inner_task_count);
    }

    static void BM_ThreadPool_NestedSpawnWait(benchmark::State& state) {
        const auto outer_task_count = static_cast<size_t>(state.range(0));
        const auto inner_task_count = static_cast<size_t>(state.range(1));
        auto bench_pool = makeBenchPool();

        for ([[maybe_unused]] auto _ : state) {
            std::atomic<uint64_t> prime_count{ 0 };
            std::atomic<bool> nested_timeout{ false };
            std::vector<Future<void>> outer_futures;
            outer_futures.reserve(outer_task_count);

            for (size_t outer_index = 0; outer_index < outer_task_count; ++outer_index) {
                outer_futures.emplace_back(bench_pool->spawn(
                    SubmitPolicy::SUBMIT_POLICY_NORMAL,
                    ExecuteFlags::EXECUTE_NORMAL,
                    [bench_pool, outer_index, inner_task_count, &prime_count, &nested_timeout](const auto&) {
                        std::vector<Future<void>> inner_futures;
                        inner_futures.reserve(inner_task_count);

                        for (size_t inner_index = 0; inner_index < inner_task_count; ++inner_index) {
                            inner_futures.emplace_back(bench_pool->spawn(
                                SubmitPolicy::SUBMIT_POLICY_FORK,
                                ExecuteFlags::EXECUTE_NORMAL,
                                [outer_index, inner_index, &prime_count](const auto&) {
                                    const auto candidate = makePrimeCandidate(outer_index, inner_index);
                                    if (isPrime(candidate)) {
                                        prime_count.fetch_add(1, std::memory_order_relaxed);
                                    }
                                }));
                        }

                        for (auto& inner_future : inner_futures) {
                            if (!waitFuture(inner_future, std::chrono::seconds(5))) {
                                nested_timeout.store(true, std::memory_order_relaxed);
                                return;
                            }
                        }
                    }));
            }

            for (auto& outer_future : outer_futures) {
                if (!waitFuture(outer_future, std::chrono::seconds(5))) {
                    state.SkipWithError("ThreadPool nested spawn wait timeout.");
                    bench_pool->stop();
                    return;
                }
            }

            if (nested_timeout.load(std::memory_order_relaxed)) {
                state.SkipWithError("ThreadPool inner nested spawn wait timeout.");
                bench_pool->stop();
                return;
            }

            benchmark::DoNotOptimize(prime_count.load(std::memory_order_relaxed));
        }

        bench_pool->stop();
        state.SetItemsProcessed(state.iterations() * outer_task_count * inner_task_count);
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

    static void BM_StdAsync_NestedSpawnWait(benchmark::State& state) {
        const auto outer_task_count = static_cast<size_t>(state.range(0));
        const auto inner_task_count = static_cast<size_t>(state.range(1));

        for ([[maybe_unused]] auto _ : state) {
            std::atomic<uint64_t> prime_count{ 0 };
            std::vector<std::future<void>> outer_futures;
            outer_futures.reserve(outer_task_count);

            for (size_t outer_index = 0; outer_index < outer_task_count; ++outer_index) {
                outer_futures.emplace_back(std::async(std::launch::async,
                    [outer_index, inner_task_count, &prime_count] {
                        std::vector<std::future<void>> inner_futures;
                        inner_futures.reserve(inner_task_count);

                        for (size_t inner_index = 0; inner_index < inner_task_count; ++inner_index) {
                            inner_futures.emplace_back(std::async(std::launch::async,
                                [outer_index, inner_index, &prime_count] {
                                    const auto candidate = makePrimeCandidate(outer_index, inner_index);
                                    if (isPrime(candidate)) {
                                        prime_count.fetch_add(1, std::memory_order_relaxed);
                                    }
                                }));
                        }

                        for (auto& inner_future : inner_futures) {
                            inner_future.wait();
                        }
                    }));
            }

            for (auto& outer_future : outer_futures) {
                outer_future.wait();
            }

            benchmark::DoNotOptimize(prime_count.load(std::memory_order_relaxed));
        }

        state.SetItemsProcessed(state.iterations() * outer_task_count * inner_task_count);
    }

    static void BM_ThreadPool_PostIdleWake(benchmark::State& state) {
        auto bench_pool = makeBenchPool();

        for ([[maybe_unused]] auto _ : state) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));

            std::latch done{ 1 };
            const auto begin = std::chrono::steady_clock::now();

            bench_pool->post(
                ExecuteFlags::EXECUTE_NORMAL,
                [&done](const auto&) {
                done.count_down();
                });
            done.wait();

            const auto elapsed = std::chrono::steady_clock::now() - begin;
            state.SetIterationTime(std::chrono::duration<double>(elapsed).count());
        }

        bench_pool->stop();
    }

    static void BM_ThreadPool_SpawnIdleWake(benchmark::State& state) {
        auto bench_pool = makeBenchPool();

        for ([[maybe_unused]] auto _ : state) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));

            std::latch done{ 1 };
            const auto begin = std::chrono::steady_clock::now();

            auto future = bench_pool->spawn(
                SubmitPolicy::SUBMIT_POLICY_NORMAL,
				ExecuteFlags::EXECUTE_NORMAL,
                [&done](const auto&) {
                done.count_down();
                });
            done.wait();
            future.wait();

            const auto elapsed = std::chrono::steady_clock::now() - begin;
            state.SetIterationTime(std::chrono::duration<double>(elapsed).count());
        }

        bench_pool->stop();
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

    void threadPoolTinyTaskArgs(benchmark::internal::Benchmark* benchmark) {
        for (const auto task_count : kTinyTaskCounts) {
            benchmark->Arg(static_cast<int64_t>(task_count));
        }
    }

    void threadPoolCpuTaskArgs(benchmark::internal::Benchmark* benchmark) {
        for (const auto task_count : kCpuTaskCounts) {
            benchmark->Args({
                static_cast<int64_t>(task_count),
                1024,
                });
        }
    }

    void threadPoolNestedTaskArgs(benchmark::internal::Benchmark* benchmark) {
        for (size_t i = 0; i < kNestedTaskCounts.size(); ++i) {
            benchmark->Args({
                static_cast<int64_t>(kNestedTaskCounts[i]),
                static_cast<int64_t>(kNestedInnerTaskCounts[i]),
                });
        }
    }

    BENCHMARK(BM_ThreadPool_CreateDestroy);
    BENCHMARK(BM_ThreadPool_PostBurstTinyTasks)
        ->Apply(threadPoolTinyTaskArgs)
        ->ArgName("tasks");
    BENCHMARK(BM_ThreadPool_SpawnBurstTinyTasks)
        ->Apply(threadPoolTinyTaskArgs)
        ->ArgName("tasks");
    BENCHMARK(BM_ThreadPool_PostBurstCpuTasks)
        ->Apply(threadPoolCpuTaskArgs)
        ->ArgNames({ "tasks", "work" });
    BENCHMARK(BM_ThreadPool_SpawnBurstCpuTasks)
        ->Apply(threadPoolCpuTaskArgs)
        ->ArgNames({ "tasks", "work" });
    BENCHMARK(BM_ThreadPool_NestedPost)
        ->Apply(threadPoolNestedTaskArgs)
        ->ArgNames({ "outer", "inner" });
    BENCHMARK(BM_ThreadPool_NestedSpawnWait)
        ->Apply(threadPoolNestedTaskArgs)
        ->ArgNames({ "outer", "inner" });
    BENCHMARK(BM_ThreadPool_PostIdleWake)
        ->Iterations(256)
        ->UseManualTime();
    BENCHMARK(BM_ThreadPool_SpawnIdleWake)
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
    BENCHMARK(BM_StdAsync_NestedSpawnWait)
        ->Args({ 4, 4 })
        ->Args({ 8, 4 })
        ->Args({ 8, 8 });
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
