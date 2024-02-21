//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/stl.h>
#include <base/task.h>
#include <base/logger_impl.h>
#include <base/ithreadpoolexecutor.h>

XAMP_BASE_NAMESPACE_BEGIN

namespace Executor {

inline constexpr size_t kDefaultParallelBatchSize = 8;

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
void ParallelFor(C& items, Func&& f, size_t batches = kDefaultParallelBatchSize) {
    auto executor = MakeThreadPoolExecutor(kDefaultThreadPoollExecutorLoggerName);
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
    size_t batches = kDefaultParallelBatchSize) {
    auto begin = std::begin(items);
    auto size = std::distance(begin, std::end(items));

    std::vector<std::pair<decltype(begin), SharedTask<void>>> futures;
    futures.reserve(batches);

    std::vector<std::pair<decltype(begin), SharedTask<void>>> timed_out_tasks;

    auto IsFutureDuplicate = [](const auto & futures, auto begin, auto i) -> bool {
        bool duplicate = false;
        for (const auto& future : futures) {
            if (future.first == begin + i) {
                duplicate = true;
                break;
            }
        }
        return duplicate;
    };

    auto IsTimedOutTasksDuplicate = [](const auto& timed_out_tasks, auto itr) -> bool {
        bool duplicate = false;
        for (const auto& f : timed_out_tasks) {
            if (f.first == itr->first) {
                duplicate = true;
                break;
            }
        }
        return duplicate;
    };

    for (size_t i = 0; i < size;) {
        size_t batch_size = (std::min)(batches, size - i);

        size_t spawn_size = 0;
        for (size_t j = 0; j < batch_size; ++j) {
            if (!IsFutureDuplicate(futures, begin, i)) {
                auto task = Executor::Spawn(executor, [f, begin, i](const StopToken&) -> void {
                    f(*(begin + i));
                    });

                futures.emplace_back(begin + i, task.share());
                ++spawn_size;
            }
        }

        i += spawn_size;

        auto itr = futures.begin();
        while (itr != futures.end()) {
            if (itr->second.wait_for(wait_timeout) == std::future_status::ready) {
                itr->second.get();
                itr = futures.erase(itr);
            }
            else {
                if (!IsTimedOutTasksDuplicate(timed_out_tasks, itr)) {
                    timed_out_tasks.push_back(*itr);
                }                
                ++itr;
            }
        }
    }

    // Wait for all timed-out tasks to complete
    for (const auto& task : timed_out_tasks) {
        task.second.wait();
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
void ParallelFor(IThreadPoolExecutor& executor, size_t begin, size_t end, Func&& f, size_t batches = kDefaultParallelBatchSize) {
    auto size = end - begin;
    for (size_t i = 0; i < size;) {
        Vector<Task<void>> futures((std::min)(size - i, batches));
        for (auto& ff : futures) {
            ff = Executor::Spawn(executor, [f, begin, i](const StopToken&) -> void {
                f(begin + i);
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

