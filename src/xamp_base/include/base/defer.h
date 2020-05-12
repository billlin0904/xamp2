//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <utility>
#include <exception>

#include <base/base.h>

namespace xamp::base {

class XAMP_BASE_API_ONLY_EXPORT UncaughtExceptionDetector {
public:
    UncaughtExceptionDetector()
        : count_(std::uncaught_exceptions()) {
    }

    operator bool() const noexcept {
        return std::uncaught_exceptions() > count_;
    }
private:
    const int32_t count_;
};

template <typename T>
class XAMP_BASE_API_ONLY_EXPORT ScopeGuard {
public:
    ScopeGuard(T&& f)
        : commit_(false)
        , f_(std::forward<T>(f)) {
    }

    template <typename Functor>
    ScopeGuard(ScopeGuard<Functor>&& other)
        : commit_(other.commit_)
        , f_(std::move(other.f_)) {
        other.Commit();
    }

    ScopeGuard() = delete;
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    ~ScopeGuard() {
        if (!commit_) {
            f_();
        }
    }

    void Commit() noexcept {
        commit_ = true;
    }
private:
    template <typename Functor>
    friend class ScopeGuard;

    mutable bool commit_;
    UncaughtExceptionDetector detector_;
    T f_;
};

template <typename F>
ScopeGuard<F> MakeScopeGuard(F &&f) {
    return ScopeGuard<F>(std::forward<F>(f));
}

}

#define XAMP_COMBIN(x, y) x##y
#define XAMP_COMBIN_NAME(x, y) XAMP_COMBIN(x, y)
#define XAMP_ANON_VAR_NAME(x) XAMP_COMBIN_NAME(x, __LINE__)

#define XAMP_ON_SCOPE_EXIT(code) \
    const auto XAMP_ANON_VAR_NAME(defer) = xamp::base::MakeScopeGuard([&]() { code; })

