//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <utility>
#include <base/base.h>

namespace xamp::base {
    
template <typename F>
class XAMP_BASE_API_ONLY_EXPORT Defer {
public:
    explicit Defer(F&& f);

    ~Defer() noexcept;

private:
    F f_;
};

template <typename F>
Defer<F>::Defer(F&& f)
    : f_(std::forward<F>(f)) {
}

template <typename F>
Defer<F>::~Defer() noexcept {
    f_();
}

template <typename F>
Defer<F> MakeDefer(F &&f) {
    return Defer<F>(std::forward<F>(f));
}

}

#define XAMP_COMBIN(x, y) x##y
#define XAMP_COMBIN_NAME(x, y) XAMP_COMBIN(x, y)
#define XAMP_DEFER_NAME(x) XAMP_COMBIN_NAME(x, __LINE__)

#define XAMP_DEFER(code) \
	const auto XAMP_DEFER_NAME(defer) = xamp::base::MakeDefer([&]() { code; })

