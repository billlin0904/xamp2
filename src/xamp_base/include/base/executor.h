//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stack>
#include <base/stl.h>
#include <base/task.h>
#include <base/logger_impl.h>
#include <base/fastconditionvariable.h>
#include <base/fastmutex.h>
#include <base/workstealingtaskqueue.h>
#include <base/ithreadpoolexecutor.h>
#include <base/blocking_queue.h>
#include <condition_variable>

XAMP_BASE_NAMESPACE_BEGIN

namespace Executor {

XAMP_DECLARE_LOG_NAME(DefaultThreadPoollExecutor);

/*
* Spawn a task.
* 
* @param[in] executor
* @param[in] f
* @param[in] args
* @return Task<decltype(f(args...))>
*/
template <typename F, typename ... Args>
decltype(auto) Spawn(IThreadPoolExecutor *executor, F&& f, Args&&... args, ExecuteFlags flags = ExecuteFlags::EXECUTE_NORMAL) {
    XAMP_ENSURES(executor != nullptr);
    return executor->Spawn(f, std::forward<Args>(args) ..., flags);
}

/*
* Parallel for.
* 
* @param[in] items
* @param[in] f
* @param[in] batches
* @return void
*/
template <typename C, typename Func>
void ParallelFor(C& items, Func&& f) {
    auto executor = ThreadPoolBuilder::MakeThreadPool(kDefaultThreadPoollExecutorLoggerName);
    ParallelFor(executor.get(), items, f);
}

#if 0
/*
* Parallel for.
* 
* @param[in] executor
* @param[in] begin
* @param[in] end
* @param[in] f
* @param[in] batches
* @return void
*/
template <typename C, typename Func>
void ParallelFor(IThreadPoolExecutor* executor,
    C& items,
    Func&& f,
    const std::chrono::milliseconds &wait_timeout = std::chrono::milliseconds(100)) {

    size_t batches = (executor->GetThreadSize() / 2) + 1;
    auto begin = items.begin();
    auto end = items.end();
    using IteratorType = decltype(begin);

    std::vector<std::pair<IteratorType, SharedTask<void>>> futures;
    futures.reserve(batches);

    std::vector<std::pair<IteratorType, SharedTask<void>>> timed_out_tasks;

    auto is_future_duplicate = [](const auto& futures, auto itr) -> bool {
        return std::find_if(futures.begin(), futures.end(), [itr](const auto& future) {
            return future.first == itr;
            }) != futures.end();
        };

    auto is_timed_out_tasks_duplicate = [](const auto& timed_out_tasks, auto itr) -> bool {
        return std::find_if(timed_out_tasks.begin(), timed_out_tasks.end(), [itr](const auto& task) {
            return task.first == itr;
            }) != timed_out_tasks.end();
        };

    for (IteratorType itr = begin; itr != end;) {
        size_t batch_size = (std::min)(batches, static_cast<size_t>(std::distance(itr, end)));

        for (size_t j = 0; j < batch_size; ++j) {
            if (!is_future_duplicate(futures, itr)) {
                auto task = Executor::Spawn(executor, [fun = std::forward<Func>(f), itr](const auto& token) -> void {
                    if (!token.stop_requested()) {
                        fun(*itr);
                    }                    
                    });

                futures.emplace_back(itr, task.share());
            }
            ++itr;
        }

        auto futures_itr = futures.begin();
        while (futures_itr != futures.end()) {
            if (futures_itr->second.wait_for(wait_timeout) == std::future_status::ready) {
                futures_itr->second.get();
                futures_itr = futures.erase(futures_itr);
            }
            else {
                if (!is_timed_out_tasks_duplicate(timed_out_tasks, futures_itr->first)) {
                    timed_out_tasks.push_back(*futures_itr);
                }
                ++futures_itr;
            }
        }
    }

    // Wait for all timed-out tasks to complete
    for (const auto& task : timed_out_tasks) {
        task.second.wait();
    }

    for (const auto& future : futures) {
        future.second.wait();
    }
}
#endif

/*
 * Parallel for.
 *
 * @param[in] executor
 * @param[in] items
 * @param[in] f
 * @return void
 *
 */
template <typename C, typename Func>
void ParallelFor(IThreadPoolExecutor* executor,
    const std::stop_token& stop_token,
    C& items,
    Func&& f) {
    using ValueType = typename C::value_type;

    if (items.empty()) {
        return;
    }

    constexpr bool can_call_with_stop =
        std::is_invocable_v<Func, ValueType&, const std::stop_token&>;

	ConcurrentQueue<ValueType*> task_queue(items.size());

    for (auto& item : items) {
        task_queue.enqueue(&item);
    }

    auto worker = [&stop_token, &task_queue, fun = std::forward<Func>(f)](const auto& token) {
        ValueType *task;
        while (!stop_token.stop_requested() && !token.stop_requested() && task_queue.try_dequeue(task)) {
            if constexpr (can_call_with_stop) {
                fun(*task, stop_token);
            } else {
                fun(*task);
            }
        }
        };

    const size_t worker_count = executor->GetThreadSize();
    std::vector<SharedTask<void>> futures;
    futures.reserve(worker_count);

    for (size_t i = 0; i < worker_count; ++i) {
        futures.push_back(Executor::Spawn(executor, worker).share());
    }

    for (auto& fut : futures) {
        fut.wait();
    }
}

/*
* Parallel for.
* 
* @param[in] executor
* @param[in] begin
* @param[in] end
* @param[in] f
* @param[in] batches
* @return void
*/
template <typename Func>
void ParallelFor(IThreadPoolExecutor* executor, size_t begin, size_t end, Func&& f) {
    size_t size = end - begin;
    size_t batches = (executor->GetThreadSize() / 2) + 1;
    for (size_t i = 0; i < size;) {
        Vector<Task<void>> futures((std::min)(size - i, batches));
        for (auto& ff : futures) {
            ff = Executor::Spawn(executor, [func = std::forward<Func>(f), begin, i](const auto& token) -> void {
                if (!token.stop_requested()) {
                    func(begin + i);
                }                
                });
            ++i;
        }
        for (auto& ff : futures) {
            ff.wait();
        }
    }
}
	
}

XAMP_BASE_NAMESPACE_END

