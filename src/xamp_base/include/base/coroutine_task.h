//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <condition_variable>
#include <coroutine>
#include <exception>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

template <typename T>
class AsyncTask {
public:
    struct Promise {
        std::optional<T> value;
        std::exception_ptr exception;
        std::coroutine_handle<> continuation;

        AsyncTask get_return_object() noexcept {
            return AsyncTask(std::coroutine_handle<Promise>::from_promise(*this));
        }

        std::suspend_always initial_suspend() const noexcept {
            return {};
        }

        auto final_suspend() noexcept {
            struct FinalAwaiter {
                bool await_ready() const noexcept {
                    return false;
                }

                std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> handle) const noexcept {
                    const auto continuation = handle.promise().continuation;
                    return continuation ? continuation : std::noop_coroutine();
                }

                void await_resume() const noexcept {
                }
            };

            return FinalAwaiter{};
        }

        template <typename U>
        void return_value(U&& result) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
            value.emplace(std::forward<U>(result));
        }

        void unhandled_exception() noexcept {
            exception = std::current_exception();
        }
    };

    using promise_type = Promise;
    using Handle = std::coroutine_handle<Promise>;

    AsyncTask() noexcept = default;

    explicit AsyncTask(Handle handle) noexcept
        : handle_(handle) {
    }

    AsyncTask(AsyncTask&& other) noexcept
        : handle_(std::exchange(other.handle_, {})) {
    }

    AsyncTask& operator=(AsyncTask&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    AsyncTask(const AsyncTask&) = delete;
    AsyncTask& operator=(const AsyncTask&) = delete;

    ~AsyncTask() {
        if (handle_) {
            handle_.destroy();
        }
    }

    void start() {
        if (handle_ && !handle_.done()) {
            handle_.resume();
        }
    }

    bool await_ready() const noexcept {
        return !handle_ || handle_.done();
    }

    void await_suspend(std::coroutine_handle<> continuation) noexcept {
        handle_.promise().continuation = continuation;
        handle_.resume();
    }

    T await_resume() {
        auto& promise = handle_.promise();
        if (promise.exception) {
            std::rethrow_exception(promise.exception);
        }
        return std::move(*promise.value);
    }

private:
    Handle handle_;
};

template <>
class AsyncTask<void> {
public:
    struct Promise {
        std::exception_ptr exception;
        std::coroutine_handle<> continuation;

        AsyncTask get_return_object() noexcept {
            return AsyncTask(std::coroutine_handle<Promise>::from_promise(*this));
        }

        std::suspend_always initial_suspend() const noexcept {
            return {};
        }

        auto final_suspend() noexcept {
            struct FinalAwaiter {
                bool await_ready() const noexcept {
                    return false;
                }

                std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> handle) const noexcept {
                    const auto continuation = handle.promise().continuation;
                    return continuation ? continuation : std::noop_coroutine();
                }

                void await_resume() const noexcept {
                }
            };

            return FinalAwaiter{};
        }

        void return_void() const noexcept {
        }

        void unhandled_exception() noexcept {
            exception = std::current_exception();
        }
    };

    using promise_type = Promise;
    using Handle = std::coroutine_handle<Promise>;

    AsyncTask() noexcept = default;

    explicit AsyncTask(Handle handle) noexcept
        : handle_(handle) {
    }

    AsyncTask(AsyncTask&& other) noexcept
        : handle_(std::exchange(other.handle_, {})) {
    }

    AsyncTask& operator=(AsyncTask&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    AsyncTask(const AsyncTask&) = delete;
    AsyncTask& operator=(const AsyncTask&) = delete;

    ~AsyncTask() {
        if (handle_) {
            handle_.destroy();
        }
    }

    void start() {
        if (handle_ && !handle_.done()) {
            handle_.resume();
        }
    }

    bool await_ready() const noexcept {
        return !handle_ || handle_.done();
    }

    void await_suspend(std::coroutine_handle<> continuation) noexcept {
        handle_.promise().continuation = continuation;
        handle_.resume();
    }

    void await_resume() {
        auto& promise = handle_.promise();
        if (promise.exception) {
            std::rethrow_exception(promise.exception);
        }
    }

private:
    Handle handle_;
};

template <typename T>
T syncWait(AsyncTask<T>&& task) {
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    std::optional<T> result;
    std::exception_ptr exception;

    auto waiter = [&]() -> AsyncTask<void> {
        try {
            result.emplace(co_await task);
        }
        catch (...) {
            exception = std::current_exception();
        }

        {
            std::lock_guard lock(mutex);
            done = true;
        }
        cv.notify_one();
    }();

    waiter.start();

    std::unique_lock lock(mutex);
    cv.wait(lock, [&] {
        return done;
    });

    if (exception) {
        std::rethrow_exception(exception);
    }
    return std::move(*result);
}

inline void syncWait(AsyncTask<void>&& task) {
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    std::exception_ptr exception;

    auto waiter = [&]() -> AsyncTask<void> {
        try {
            co_await task;
        }
        catch (...) {
            exception = std::current_exception();
        }

        {
            std::lock_guard lock(mutex);
            done = true;
        }
        cv.notify_one();
    }();

    waiter.start();

    std::unique_lock lock(mutex);
    cv.wait(lock, [&] {
        return done;
    });

    if (exception) {
        std::rethrow_exception(exception);
    }
}

XAMP_BASE_NAMESPACE_END
