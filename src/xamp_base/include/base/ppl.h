//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/stl.h>
#include <base/ithreadpool.h>

namespace xamp::base {

template <typename C, typename Func>
void ParallelFor(C& items, Func&& f, IThreadPoolExcutor& tp, size_t batches = 8) {
    auto begin = std::begin(items);
    auto size = std::distance(begin, std::end(items));

    for (size_t i = 0; i < size; ++i) {
        Vector<Task<void>> futures((std::min)(size - i,batches));
        for (auto& ff : futures) {
            ff = tp.Spawn([f, begin, i](size_t) -> void {
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
void ParallelFor(size_t begin, size_t end, Func &&f, IThreadPoolExcutor& tp, size_t batches = 4) {
    auto size = end - begin;
    for (size_t i = 0; i < size; ++i) {
        Vector<Task<void>> futures((std::min)(size - i, batches));
        for (auto& ff : futures) {
            ff = tp.Spawn([f, i](size_t) -> void {
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

