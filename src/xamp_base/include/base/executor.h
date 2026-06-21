//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <condition_variable>
#include <optional>
#include <stack>
#include <type_traits>
#include <vector>

#include <base/stl.h>
#include <base/task.h>
#include <base/logger.h>
#include <base/fastconditionvariable.h>
#include <base/fastmutex.h>
#include <base/workstealingtaskqueue.h>
#include <base/threadpool.h>
#include <base/blocking_queue.h>

XAMP_BASE_NAMESPACE_BEGIN

namespace Executor {

namespace detail {

template <typename Func, typename ValueType>
inline constexpr bool kParallelForCallableWithStopToken =
    std::is_invocable_v<Func&, ValueType&, const std::stop_token&>;

template <bool WithStopToken, typename Func, typename ValueType>
struct ParallelForInvokeResult;

template <typename Func, typename ValueType>
struct ParallelForInvokeResult<true, Func, ValueType> {
    using Type = std::invoke_result_t<Func&, ValueType&, const std::stop_token&>;
};

template <typename Func, typename ValueType>
struct ParallelForInvokeResult<false, Func, ValueType> {
    using Type = std::invoke_result_t<Func&, ValueType&>;
};

template <typename Func, typename ValueType>
using ParallelForResult = std::remove_cvref_t<typename ParallelForInvokeResult<
    kParallelForCallableWithStopToken<Func, ValueType>,
    Func,
    ValueType>::Type>;

} // namespace detail

template <typename C, typename Func>
void parallelFor(const std::shared_ptr<IThreadPool>& executor,
    C& items,
    Func&& f,
    const std::stop_token& stop_token = std::stop_token(),
    bool is_fork = false)
    requires std::is_void_v<detail::ParallelForResult<std::decay_t<Func>, typename C::value_type>> {
    using ValueType = typename C::value_type;

    if (items.empty()) {
        return;
    }

    constexpr bool can_call_with_stop =
        detail::kParallelForCallableWithStopToken<std::decay_t<Func>, ValueType>;

    ConcurrentQueue<ValueType*> task_queue(items.size());

    for (auto& item : items) {
        task_queue.enqueue(&item);
    }

    auto worker =
        [&stop_token, &task_queue, fun = std::forward<Func>(f)](const auto& token) mutable {
        ValueType* task;
        while (!stop_token.stop_requested()
            && !token.stop_requested()
            && task_queue.try_dequeue(task)) {
            if constexpr (can_call_with_stop) {
                fun(*task, stop_token);
            }
            else {
                fun(*task);
            }
        }
        };

    const auto worker_count = executor->getThreadSize();

    std::vector<SharedFuture<void>> futures;
    futures.reserve(worker_count);

    for (size_t i = 0; i < worker_count; ++i) {
        futures.push_back(executor->spawn(
            is_fork ? SubmitPolicy::SUBMIT_POLICY_FORK : SubmitPolicy::SUBMIT_POLICY_NORMAL,
            ExecuteFlags::EXECUTE_NORMAL,
            worker).share());
    }

    for (auto& fut : futures) {
        fut.wait();
        fut.get();
    }
}

template <typename C, typename Func>
[[nodiscard]] auto parallelFor(const std::shared_ptr<IThreadPool>& executor,
    C& items,
    Func&& f,
    const std::stop_token& stop_token = std::stop_token(),
    bool is_fork = false)
    -> std::vector<std::optional<detail::ParallelForResult<std::decay_t<Func>, typename C::value_type>>>
    requires (!std::is_void_v<detail::ParallelForResult<std::decay_t<Func>, typename C::value_type>>) {
    using ValueType = typename C::value_type;
    using ResultType = detail::ParallelForResult<std::decay_t<Func>, ValueType>;

    if (items.empty()) {
        return {};
    }

    constexpr bool can_call_with_stop =
        detail::kParallelForCallableWithStopToken<std::decay_t<Func>, ValueType>;

    struct WorkItem final {
        size_t     index;
        ValueType* value;
    };

    ConcurrentQueue<WorkItem> task_queue(items.size());

    size_t index = 0;
    for (auto& item : items) {
        task_queue.enqueue(WorkItem{ index++, &item });
    }

    struct WorkResult final {
        size_t index;
        ResultType value;
    };
    using WorkerResults = std::vector<WorkResult>;

    auto worker =
        [&stop_token, &task_queue, fun = std::forward<Func>(f)](const auto& token) mutable -> WorkerResults {
        WorkerResults local_results;
        WorkItem task;
        while (!stop_token.stop_requested()
            && !token.stop_requested()
            && task_queue.try_dequeue(task)) {
            if constexpr (can_call_with_stop) {
                local_results.push_back(WorkResult{ task.index, fun(*task.value, stop_token) });
            }
            else {
                local_results.push_back(WorkResult{ task.index, fun(*task.value) });
            }
        }
        return local_results;
        };

    const auto worker_count = executor->getThreadSize();

    std::vector<Future<WorkerResults>> futures;
    futures.reserve(worker_count);

    for (size_t i = 0; i < worker_count; ++i) {
        futures.push_back(executor->spawn(
            is_fork ? SubmitPolicy::SUBMIT_POLICY_FORK : SubmitPolicy::SUBMIT_POLICY_NORMAL,
            ExecuteFlags::EXECUTE_NORMAL,
            worker));
    }

    std::vector<std::optional<ResultType>> results(items.size());
    for (auto& fut : futures) {
        auto worker_results = fut.get();
        for (auto& result : worker_results) {
            results[result.index].emplace(std::move(result.value));
        }
    }

    return results;
}

template <typename C, typename Func>
void parallelForEach(const std::shared_ptr<IThreadPool>& executor,
    C& items,
    Func&& f,
    const std::stop_token& stop_token = std::stop_token()) {
    using IteratorType = typename C::iterator;
    using ValueType = typename C::value_type;
    IteratorType begin = items.begin();
    IteratorType end = items.end();
    size_t size = std::distance(begin, end);
    size_t batches = 8;

    constexpr bool can_call_with_stop =
        std::is_invocable_v<Func, ValueType&, const std::stop_token&>;
    auto itr = begin;
    for (size_t i = 0; i < size;) {
        size_t batch_size = (std::min)(batches, static_cast<size_t>(std::distance(itr, end)));
        std::vector<Future<void>> futures((std::min)(size - i, batches));
        for (auto& ff : futures) {
            ff = executor->spawn(
                SubmitPolicy::SUBMIT_POLICY_NORMAL,
                ExecuteFlags::EXECUTE_NORMAL,
                [func = std::forward<Func>(f), itr](const auto& token) -> void {
                    if constexpr (can_call_with_stop) {
                        func(*itr, token);
                    }
                    else {
                        func(*itr);
                    }
                });
            ++i;
            ++itr;
        }
        for (auto& ff : futures) {
            ff.wait();
            ff.get();
        }
    }
}

template <typename Func>
void parallelForEach(const std::shared_ptr<IThreadPool>& executor, size_t begin, size_t end, Func&& f) {
    size_t size = end - begin;
    size_t batches = (executor->getThreadSize() / 2) + 1;

    constexpr bool can_call_with_stop =
        std::is_invocable_v<Func, size_t, const std::stop_token&>;

    for (size_t i = 0; i < size;) {
        std::vector<Future<void>> futures((std::min)(size - i, batches));
        for (auto& ff : futures) {
            ff = executor->spawn(
                SubmitPolicy::SUBMIT_POLICY_NORMAL,
                ExecuteFlags::EXECUTE_NORMAL,
                [func = std::forward<Func>(f), begin, i](const auto& token) -> void {
                if constexpr (can_call_with_stop) {
                    func(begin + i, token);
                }
                else {
                    func(begin + i);
                }
                });
            ++i;
        }
        for (auto& ff : futures) {
            ff.wait();
            ff.get();
        }
    }
}

}

XAMP_BASE_NAMESPACE_END