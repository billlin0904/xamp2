//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/stl.h>
#include <base/task.h>
#include <base/ithreadpoolexecutor.h>

namespace xamp::base {

namespace Executor {

inline constexpr size_t kDefaultParallelBatchSize = 8;

XAMP_DECLARE_LOG_NAME(DefaultThreadPoollExecutor);

template <typename F, typename ... Args>
decltype(auto) Spawn(IThreadPoolExecutor& executor, F&& f, Args&&... args) {
    return executor.Spawn(f, std::forward<Args>(args) ...);
}

template <typename C, typename Func>
void ParallelFor(C& items, Func&& f, size_t batches = kDefaultParallelBatchSize) {
    auto executor = MakeThreadPoolExecutor(kDefaultThreadPoollExecutorLoggerName);
    ParallelFor(*executor, items, f, batches);
}

template <typename C, typename Func>
void ParallelFor(IThreadPoolExecutor& executor, C& items, Func&& f, size_t batches = kDefaultParallelBatchSize) {
    auto begin = std::begin(items);
    auto size = std::distance(begin, std::end(items));

    for (size_t i = 0; i < size;) {
        Vector<Task<void>> futures((std::min)(size - i, batches));
        for (auto& ff : futures) {
            ff = Executor::Spawn(executor, [f, begin, i]() -> void {
                f(*(begin + i));
                });
            ++i;
        }
        for (auto& ff : futures) {
            ff.wait();
        }
    }
}

template <typename Func>
void ParallelFor(IThreadPoolExecutor& executor, size_t begin, size_t end, Func&& f, size_t batches = kDefaultParallelBatchSize) {
    auto size = end - begin;
    for (size_t i = 0; i < size;) {
        Vector<Task<void>> futures((std::min)(size - i, batches));
        for (auto& ff : futures) {
            ff = Executor::Spawn(executor, [f, i]() -> void {
                f(i);
                });
            ++i;
        }
        for (auto& ff : futures) {
            ff.wait();
        }
    }
}
	
}

}

