//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <coroutine>
#include <memory>
#include <utility>

#include <base/base.h>
#include <base/threadpool.h>

XAMP_BASE_NAMESPACE_BEGIN

class ThreadPoolScheduleAwaiter {
public:
    ThreadPoolScheduleAwaiter(std::shared_ptr<IThreadPool> pool,
        SubmitPolicy policy,
        ExecuteFlags flags) noexcept
        : pool_(std::move(pool))
        , policy_(policy)
        , flags_(flags) {
    }

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) const {
        pool_->resumeCoroutine(policy_, flags_, handle);
    }

    void await_resume() const noexcept {
    }

private:
    std::shared_ptr<IThreadPool> pool_;
    SubmitPolicy policy_;
    ExecuteFlags flags_;
};

inline ThreadPoolScheduleAwaiter scheduleOn(std::shared_ptr<IThreadPool> pool,
    SubmitPolicy policy = SubmitPolicy::SUBMIT_POLICY_NORMAL,
    ExecuteFlags flags = ExecuteFlags::EXECUTE_NORMAL) noexcept {
    return ThreadPoolScheduleAwaiter(std::move(pool), policy, flags);
}

XAMP_BASE_NAMESPACE_END
