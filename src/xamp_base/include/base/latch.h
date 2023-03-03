//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <atomic>
#include <mutex>

#ifdef __cpp_lib_latch
#include <latch>
#endif

#include <base/fastconditionvariable.h>
#include <base/fastmutex.h>

namespace xamp::base {

#ifdef __cpp_lib_latch

using Latch = std::latch;

#else

class Latch {
public:
    explicit Latch(std::ptrdiff_t const def = 1) noexcept
		: counter_(def) {
    }

    void count_down(std::ptrdiff_t const update = 1) noexcept {
	    const auto old = counter_.fetch_sub(update);
        if (old == update) {
            cv_.notify_all();
        }
    }

    void wait() const {
        if (counter_.load(std::memory_order_relaxed) == 0) {
            return;
        }

        std::unique_lock<FastMutex> lock(mut_);
        cv_.wait(lock, [this] {
	        return counter_.load(std::memory_order_relaxed) == 0;
        });
    }

    bool try_wait() const noexcept {
        return counter_.load(std::memory_order_relaxed) == 0;
    }

    void arrive_and_wait(std::ptrdiff_t const n = 1) {
        count_down(n);
        wait();
    }

    static constexpr std::ptrdiff_t max() noexcept {
        return std::numeric_limits<std::ptrdiff_t>::max();
    }
private:
    std::atomic<std::ptrdiff_t> counter_;
    mutable FastConditionVariable cv_;
    mutable FastMutex mut_;
};

#endif

}
