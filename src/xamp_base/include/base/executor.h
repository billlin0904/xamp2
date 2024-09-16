//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stack>
#include <base/stl.h>
#include <base/task.h>
#include <base/logger_impl.h>
#include <base/fastmutex.h>
#include <base/ithreadpoolexecutor.h>

#include <condition_variable>

XAMP_BASE_NAMESPACE_BEGIN

namespace Executor {

inline constexpr size_t kBatchSize = 8;

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
decltype(auto) Spawn(IThreadPoolExecutor& executor, F&& f, Args&&... args, ExecuteFlags flags = ExecuteFlags::EXECUTE_NORMAL) {
    return executor.Spawn(f, std::forward<Args>(args) ..., flags);
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
void ParallelFor(C& items, Func&& f, size_t batches = kBatchSize) {
    auto executor = ThreadPoolBuilder::MakeThreadPool(kDefaultThreadPoollExecutorLoggerName);
    ParallelFor(*executor, items, f, batches);
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
template <typename C, typename Func>
void ParallelFor(IThreadPoolExecutor& executor,
    C& items,
    Func&& f,
    const std::chrono::milliseconds &wait_timeout = std::chrono::milliseconds(100),
    size_t batches = kBatchSize) {
    using IteratorType = typename C::iterator;

    IteratorType begin = items.begin();
    IteratorType end = items.end();

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
                auto task = Executor::Spawn(executor, [fun = std::forward<Func>(f), itr](const StopToken& token) -> void {
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
void ParallelFor(IThreadPoolExecutor& executor, size_t begin, size_t end, Func&& f, size_t batches = kBatchSize) {    
    size_t size = end - begin;
    for (size_t i = 0; i < size;) {
        Vector<Task<void>> futures((std::min)(size - i, batches));
        for (auto& ff : futures) {
            ff = Executor::Spawn(executor, [f, begin, i](const StopToken& token) -> void {
                if (!token.stop_requested()) {
                    f(begin + i);
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

