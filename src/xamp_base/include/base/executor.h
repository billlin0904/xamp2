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

