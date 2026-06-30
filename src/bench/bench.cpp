#include <benchmark/benchmark.h>

#include <base/fastiostream.h>
#include <base/fastconditionvariable.h>
#include <base/fastmutex.h>
#include <base/str_utilts.h>
#include <base/threadpool.h>
#include <base/threadpoolbuilder.h>
#include <base/logger.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <future>
#include <latch>
#include <memory>
#include <mutex>
#include <array>
#include <string>
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
    constexpr std::array<size_t, 4> kConditionVariablePingPongCounts{ 1, 16, 256, 4096 };
    constexpr std::array<size_t, 4> kWideUtf8CodeUnitCounts{ 16, 256, 4096, 65536 };
    constexpr std::array<size_t, 2> kFastIOBenchFileSizes{ 4 * 1024 * 1024, 64 * 1024 * 1024 };
    constexpr std::array<size_t, 2> kFastIOSequentialChunkSizes{ 4 * 1024, 64 * 1024 };
    constexpr std::array<size_t, 2> kFastIOSeekCounts{ 1024, 4096 };
    constexpr size_t kFastIOSeekReadSize = 4 * 1024;

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

    std::wstring makeWideUtf8BenchInput(size_t code_unit_count) {
        const auto pattern =
            L"ASCII-1234567890 "
            L"\u3042\u306A\u305F\u306B\u51FA\u4F1A\u308F\u306A\u3051\u308C\u3070 "
            L"\u8A18\u61B6\u306A\u3069\u3044\u3089\u306A\u3044 "
            L"\u6C38\u9060\u306B\u7720\u308A\u305F\u3044 ";

        std::wstring input;
        input.reserve(code_unit_count);
        while (input.size() < code_unit_count) {
            input.append(pattern);
        }
        input.resize(code_unit_count);
        return input;
    }

    Path makeFastIOBenchFile(size_t file_size) {
        auto path = Fs::temp_directory_path()
            / String::format("xamp_fastiostream_bench_{}.bin", file_size);

        if (Fs::exists(path) && Fs::file_size(path) == file_size) {
            return path;
        }

        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            throw std::runtime_error("Can't create FastIOStream benchmark file.");
        }

        std::array<char, 64 * 1024> buffer{};
        for (size_t i = 0; i < buffer.size(); ++i) {
            buffer[i] = static_cast<char>((i * 131 + 17) & 0xFF);
        }

        size_t written = 0;
        while (written < file_size) {
            const auto write_size = std::min(buffer.size(), file_size - written);
            file.write(buffer.data(), static_cast<std::streamsize>(write_size));
            written += write_size;
        }

        return path;
    }

    std::vector<uint64_t> makeFastIOSeekOffsets(size_t file_size, size_t seek_count, size_t read_size) {
        std::vector<uint64_t> offsets;
        offsets.reserve(seek_count);

        const auto max_offset = file_size > read_size ? file_size - read_size : 0;
        for (size_t i = 0; i < seek_count; ++i) {
            const auto mixed = static_cast<uint64_t>(i) * 11400714819323198485ull + 0x9E3779B97F4A7C15ull;
            offsets.push_back(max_offset == 0 ? 0 : mixed % (max_offset + 1));
        }
        return offsets;
    }

#ifdef _WIN32
    std::string wideCharToUtf8String(const std::wstring& input) {
        if (input.empty()) {
            return {};
        }

        const auto input_size = static_cast<int>(input.size());
        const auto output_size = ::WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            input.data(),
            input_size,
            nullptr,
            0,
            nullptr,
            nullptr);
        if (output_size <= 0) {
            return {};
        }

        std::string output;
        output.resize(output_size);
        const auto converted_size = ::WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            input.data(),
            input_size,
            output.data(),
            output_size,
            nullptr,
            nullptr);
        if (converted_size <= 0) {
            return {};
        }
        return output;
    }
#endif

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
                future.get();
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
                future.get();
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
                future.get();
            }

            benchmark::DoNotOptimize(completed.load(std::memory_order_relaxed));
        }

        state.SetItemsProcessed(state.iterations() * task_count);
    }

    static void BM_StdAsync_SpawnBurstCpuTasks(benchmark::State& state) {
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
                future.get();
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
                            inner_future.get();
                        }
                    }));
            }

            for (auto& outer_future : outer_futures) {
                outer_future.wait();
                outer_future.get();
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
            future.get();

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
            future.get();

            const auto elapsed = std::chrono::steady_clock::now() - begin;
            state.SetIterationTime(std::chrono::duration<double>(elapsed).count());
        }
    }

    template <typename ConditionVariable, typename Mutex>
    void conditionVariablePingPong(benchmark::State& state) {
        const auto handoff_count = static_cast<size_t>(state.range(0));

        Mutex mutex;
        ConditionVariable cv;
        auto ready = false;
        auto acknowledged = false;
        auto stop = false;

        std::thread waiter([&]() {
            std::unique_lock<Mutex> lock(mutex);
            for (;;) {
                cv.wait(lock, [&]() {
                    return ready || stop;
                    });
                if (stop) {
                    break;
                }

                ready = false;
                acknowledged = true;
                cv.notify_one();
            }
            });

        for ([[maybe_unused]] auto _ : state) {
            for (size_t i = 0; i < handoff_count; ++i) {
                std::unique_lock<Mutex> lock(mutex);
                ready = true;
                acknowledged = false;
                cv.notify_one();
                cv.wait(lock, [&]() {
                    return acknowledged;
                    });
            }

            benchmark::DoNotOptimize(acknowledged);
        }

        state.PauseTiming();
        {
            std::unique_lock<Mutex> lock(mutex);
            stop = true;
            cv.notify_one();
        }
        waiter.join();
        state.ResumeTiming();

        state.SetItemsProcessed(state.iterations() * handoff_count);
    }

    static void BM_StdConditionVariable_PingPong(benchmark::State& state) {
        conditionVariablePingPong<std::condition_variable, std::mutex>(state);
    }

    static void BM_FastConditionVariable_PingPong(benchmark::State& state) {
        conditionVariablePingPong<FastConditionVariable, FastMutex>(state);
    }

    static void BM_String_ToUtf8String(benchmark::State& state) {
        const auto input = makeWideUtf8BenchInput(static_cast<size_t>(state.range(0)));

        for ([[maybe_unused]] auto _ : state) {
            auto output = String::toUtf8String(input);
            benchmark::DoNotOptimize(output.data());
            benchmark::DoNotOptimize(output.size());
        }

        state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(input.size() * sizeof(wchar_t)));
        state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(input.size()));
    }

    static void BM_Win32_WideCharToMultiByte(benchmark::State& state) {
#ifdef _WIN32
        const auto input = makeWideUtf8BenchInput(static_cast<size_t>(state.range(0)));

        for ([[maybe_unused]] auto _ : state) {
            auto output = wideCharToUtf8String(input);
            benchmark::DoNotOptimize(output.data());
            benchmark::DoNotOptimize(output.size());
        }

        state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(input.size() * sizeof(wchar_t)));
        state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(input.size()));
#else
        state.SkipWithError("WideCharToMultiByte is only available on Windows.");
#endif
    }

    static void BM_FastIOStream_SequentialRead(benchmark::State& state) {
        const auto file_size = static_cast<size_t>(state.range(0));
        const auto chunk_size = static_cast<size_t>(state.range(1));
        const auto path = makeFastIOBenchFile(file_size);
        std::vector<char> buffer(chunk_size);
        FastIOStream stream(path);

        for ([[maybe_unused]] auto _ : state) {
            stream.seek(0, SEEK_SET);

            size_t total_read = 0;
            while (total_read < file_size) {
                const auto read_size = std::min(chunk_size, file_size - total_read);
                const auto bytes_read = stream.read(buffer.data(), read_size);
                if (bytes_read == 0) {
                    break;
                }
                total_read += bytes_read;
                benchmark::DoNotOptimize(buffer.data());
            }

            benchmark::DoNotOptimize(total_read);
        }

        state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(file_size));
    }

    static void BM_StdIfstream_SequentialRead(benchmark::State& state) {
        const auto file_size = static_cast<size_t>(state.range(0));
        const auto chunk_size = static_cast<size_t>(state.range(1));
        const auto path = makeFastIOBenchFile(file_size);
        std::vector<char> buffer(chunk_size);
        std::ifstream stream(path, std::ios::binary);

        for ([[maybe_unused]] auto _ : state) {
            stream.clear();
            stream.seekg(0, std::ios::beg);

            size_t total_read = 0;
            while (total_read < file_size) {
                const auto read_size = std::min(chunk_size, file_size - total_read);
                stream.read(buffer.data(), static_cast<std::streamsize>(read_size));
                const auto bytes_read = static_cast<size_t>(stream.gcount());
                if (bytes_read == 0) {
                    break;
                }
                total_read += bytes_read;
                benchmark::DoNotOptimize(buffer.data());
            }

            benchmark::DoNotOptimize(total_read);
        }

        state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(file_size));
    }

    static void BM_FastIOStream_RandomSeekRead(benchmark::State& state) {
        const auto file_size = static_cast<size_t>(state.range(0));
        const auto seek_count = static_cast<size_t>(state.range(1));
        const auto read_size = static_cast<size_t>(state.range(2));
        const auto path = makeFastIOBenchFile(file_size);
        const auto offsets = makeFastIOSeekOffsets(file_size, seek_count, read_size);
        std::vector<char> buffer(read_size);
        FastIOStream stream(path);

        for ([[maybe_unused]] auto _ : state) {
            size_t total_read = 0;
            for (const auto offset : offsets) {
                stream.seek(static_cast<int64_t>(offset), SEEK_SET);
                total_read += stream.read(buffer.data(), read_size);
                benchmark::DoNotOptimize(buffer.data());
            }
            benchmark::DoNotOptimize(total_read);
        }

        state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(seek_count * read_size));
        state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(seek_count));
    }

    static void BM_StdIfstream_RandomSeekRead(benchmark::State& state) {
        const auto file_size = static_cast<size_t>(state.range(0));
        const auto seek_count = static_cast<size_t>(state.range(1));
        const auto read_size = static_cast<size_t>(state.range(2));
        const auto path = makeFastIOBenchFile(file_size);
        const auto offsets = makeFastIOSeekOffsets(file_size, seek_count, read_size);
        std::vector<char> buffer(read_size);
        std::ifstream stream(path, std::ios::binary);

        for ([[maybe_unused]] auto _ : state) {
            size_t total_read = 0;
            for (const auto offset : offsets) {
                stream.clear();
                stream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
                stream.read(buffer.data(), static_cast<std::streamsize>(read_size));
                total_read += static_cast<size_t>(stream.gcount());
                benchmark::DoNotOptimize(buffer.data());
            }
            benchmark::DoNotOptimize(total_read);
        }

        state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(seek_count * read_size));
        state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(seek_count));
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

    void conditionVariablePingPongArgs(benchmark::internal::Benchmark* benchmark) {
        for (const auto handoff_count : kConditionVariablePingPongCounts) {
            benchmark->Arg(static_cast<int64_t>(handoff_count));
        }
    }

    void wideUtf8Args(benchmark::internal::Benchmark* benchmark) {
        for (const auto code_unit_count : kWideUtf8CodeUnitCounts) {
            benchmark->Arg(static_cast<int64_t>(code_unit_count));
        }
    }

    void fastIOSequentialArgs(benchmark::internal::Benchmark* benchmark) {
        for (const auto file_size : kFastIOBenchFileSizes) {
            for (const auto chunk_size : kFastIOSequentialChunkSizes) {
                benchmark->Args({
                    static_cast<int64_t>(file_size),
                    static_cast<int64_t>(chunk_size),
                    });
            }
        }
    }

    void fastIOSeekArgs(benchmark::internal::Benchmark* benchmark) {
        for (const auto file_size : kFastIOBenchFileSizes) {
            for (const auto seek_count : kFastIOSeekCounts) {
                benchmark->Args({
                    static_cast<int64_t>(file_size),
                    static_cast<int64_t>(seek_count),
                    static_cast<int64_t>(kFastIOSeekReadSize),
                    });
            }
        }
    }

    BENCHMARK(BM_ThreadPool_SpawnBurstTinyTasks)
        ->Apply(threadPoolCpuTaskArgs)
        ->ArgNames({ "tasks", "work" });
    BENCHMARK(BM_StdAsync_BurstTinyTasks)
        ->Apply(threadPoolCpuTaskArgs)
        ->ArgNames({ "tasks", "work" });

    BENCHMARK(BM_ThreadPool_SpawnBurstCpuTasks)
        ->Apply(threadPoolCpuTaskArgs)
        ->ArgNames({ "tasks", "work" });
    BENCHMARK(BM_StdAsync_SpawnBurstCpuTasks)
        ->Apply(threadPoolCpuTaskArgs)
        ->ArgNames({ "tasks", "work" });

    BENCHMARK(BM_StdConditionVariable_PingPong)
        ->Apply(conditionVariablePingPongArgs)
        ->ArgName("handoffs")
        ->UseRealTime();
    BENCHMARK(BM_FastConditionVariable_PingPong)
        ->Apply(conditionVariablePingPongArgs)
        ->ArgName("handoffs")
        ->UseRealTime();

    BENCHMARK(BM_String_ToUtf8String)
        ->Apply(wideUtf8Args)
        ->ArgName("wchars");
    BENCHMARK(BM_Win32_WideCharToMultiByte)
        ->Apply(wideUtf8Args)
        ->ArgName("wchars");

    BENCHMARK(BM_FastIOStream_SequentialRead)
        ->Apply(fastIOSequentialArgs)
        ->ArgNames({ "file_bytes", "chunk_bytes" });
    BENCHMARK(BM_StdIfstream_SequentialRead)
        ->Apply(fastIOSequentialArgs)
        ->ArgNames({ "file_bytes", "chunk_bytes" });

    BENCHMARK(BM_FastIOStream_RandomSeekRead)
        ->Apply(fastIOSeekArgs)
        ->ArgNames({ "file_bytes", "seeks", "read_bytes" });
    BENCHMARK(BM_StdIfstream_RandomSeekRead)
        ->Apply(fastIOSeekArgs)
        ->ArgNames({ "file_bytes", "seeks", "read_bytes" });
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
