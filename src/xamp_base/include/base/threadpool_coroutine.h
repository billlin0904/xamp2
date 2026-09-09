//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <stop_token>
#include <type_traits>
#include <utility>

#include <base/base.h>
#include <base/threadpool.h>

XAMP_BASE_NAMESPACE_BEGIN

namespace detail {

template <bool WithStopToken, typename F>
struct ThreadPoolSubmitReturn;

template <typename F>
struct ThreadPoolSubmitReturn<true, F> {
    using Type = std::invoke_result_t<F&, const std::stop_token&>;
};

template <typename F>
struct ThreadPoolSubmitReturn<false, F> {
    using Type = std::invoke_result_t<F&>;
};

template <typename T>
struct ThreadPoolSubmitState {
    std::optional<T> value;
    std::exception_ptr exception;
};

template <>
struct ThreadPoolSubmitState<void> {
    bool completed{ false };
    std::exception_ptr exception;
};

} // namespace detail

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

template <typename F>
class ThreadPoolSubmitAwaiter {
public:
    static constexpr bool kAcceptsStopToken = std::is_invocable_v<F&, const std::stop_token&>;
    using Result = typename detail::ThreadPoolSubmitReturn<kAcceptsStopToken, F>::Type;

    ThreadPoolSubmitAwaiter(std::shared_ptr<IThreadPool> pool,
        F&& func,
        SubmitPolicy policy,
        ExecuteFlags flags)
        : pool_(std::move(pool))
        , func_(std::move(func))
        , policy_(policy)
        , flags_(flags)
        , state_(std::make_shared<detail::ThreadPoolSubmitState<Result>>()) {
    }

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) {
        auto state = state_;
        auto func = std::move(func_);

        pool_->submitCoroutine(policy_,
            flags_,
            [state,
             func = std::move(func),
             handle](const std::stop_token& stop_token) mutable {
                try {
                    if constexpr (std::is_void_v<Result>) {
                        if constexpr (kAcceptsStopToken) {
                            std::invoke(func, stop_token);
                        }
                        else {
                            std::invoke(func);
                        }
                        state->completed = true;
                    }
                    else {
                        if constexpr (kAcceptsStopToken) {
                            state->value.emplace(std::invoke(func, stop_token));
                        }
                        else {
                            state->value.emplace(std::invoke(func));
                        }
                    }
                }
                catch (...) {
                    state->exception = std::current_exception();
                }
                handle.resume();
            });
    }

    Result await_resume() {
        if (state_->exception) {
            std::rethrow_exception(state_->exception);
        }

        if constexpr (std::is_void_v<Result>) {
            return;
        }
        else {
            return std::move(*state_->value);
        }
    }

private:
    std::shared_ptr<IThreadPool> pool_;
    F func_;
    SubmitPolicy policy_;
    ExecuteFlags flags_;
    std::shared_ptr<detail::ThreadPoolSubmitState<Result>> state_;
};

inline ThreadPoolScheduleAwaiter scheduleOn(std::shared_ptr<IThreadPool> pool,
    SubmitPolicy policy = SubmitPolicy::SUBMIT_POLICY_NORMAL,
    ExecuteFlags flags = ExecuteFlags::EXECUTE_NORMAL) noexcept {
    return ThreadPoolScheduleAwaiter(std::move(pool), policy, flags);
}

template <typename F>
auto submitOn(std::shared_ptr<IThreadPool> pool,
    F&& func,
    SubmitPolicy policy = SubmitPolicy::SUBMIT_POLICY_NORMAL,
    ExecuteFlags flags = ExecuteFlags::EXECUTE_NORMAL) {
    return ThreadPoolSubmitAwaiter<std::decay_t<F>>(
        std::move(pool),
        std::forward<F>(func),
        policy,
        flags);
}

XAMP_BASE_NAMESPACE_END
