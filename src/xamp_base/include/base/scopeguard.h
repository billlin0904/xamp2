//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <utility>
#include <exception>

#include <base/base.h>

namespace xamp::base {

class XAMP_BASE_API_ONLY_EXPORT UncaughtExceptionDetector final {
public:
    UncaughtExceptionDetector() noexcept
        : count_(std::uncaught_exceptions()) {
    }

    operator bool() const noexcept {
        return std::uncaught_exceptions() > count_;
    }
private:
    const int32_t count_;
};

// C++17 ScopeGuard
template <typename T, bool ExceptedSuccess, bool ExceptedFailure>
class XAMP_BASE_API_ONLY_EXPORT ScopeGuard final {
public:
    ScopeGuard(T&& f) noexcept(
        std::is_nothrow_move_constructible_v<T> ||
        std::is_nothrow_copy_constructible_v<T>)
        : f_(std::forward<T>(f)) {
    }

    ScopeGuard(ScopeGuard&& other)noexcept(
        std::is_nothrow_move_constructible_v<T> ||
        std::is_nothrow_copy_constructible_v<T>)
        : f_(std::move_if_noexcept(other.f_)) {
    }

    ScopeGuard() = delete;

    XAMP_DISABLE_COPY(ScopeGuard)

    ~ScopeGuard() noexcept {
        if ((ExceptedSuccess && !detector_) || (ExceptedFailure && detector_)) {
            f_();
        }
    }
private:
    UncaughtExceptionDetector detector_;
    T f_;
};

struct ScopeGuardOnSuccess {};

struct ScopeGuardOnFailure {};

template <typename Func>
auto operator+(ScopeGuardOnSuccess, Func&& func) {
    return ScopeGuard<Func, true, false>(std::forward<Func>(func));
}

template <typename Func>
auto operator+(ScopeGuardOnFailure, Func&& func) {
    return ScopeGuard<Func, false, true>(std::forward<Func>(func));
}

}

#define XAMP_ON_SCOPE_EXIT(code) \
    const auto XAMP_ANON_VAR_NAME(__scope_guard_) = xamp::base::ScopeGuardOnSuccess() + ([&]() { code; })

#define XAMP_ON_SCOPE_FAIL(code) \
    const auto XAMP_ANON_VAR_NAME(__scope_guard_) = xamp::base::ScopeGuardOnFailure() + ([&]() { code; })

