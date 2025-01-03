//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <algorithm>
#include <stop_token>
#include <future>
#include <functional>

#include <base/base.h>
#include <base/memory.h>
#include <base/assert.h>

XAMP_BASE_NAMESPACE_BEGIN
/*
* MoveOnlyFunction is a function wrapper that can only be moved.
* It is used to wrap a function that is only called once.
*/
class MoveOnlyFunction final {
public:
    MoveOnlyFunction() noexcept = default;

    template <typename Func>
    MoveOnlyFunction(Func&& f)
        : impl_(MakeAlign<ImplBase, ImplType<Func>>(std::forward<Func>(f))) {
    }

	/*
	 * Calls the wrapped function with the given stop_token.
	 * The function is only called once.
	 */
    XAMP_ALWAYS_INLINE void operator()(const std::stop_token& stop_token) {
        XAMP_EXPECTS(impl_ != nullptr);
	    impl_->Invoke(stop_token);
        impl_.reset();
    }

    MoveOnlyFunction(MoveOnlyFunction&& other) noexcept
		: impl_(std::move(other.impl_)) {	    
    }

    MoveOnlyFunction& operator=(MoveOnlyFunction&& other) noexcept {
        impl_ = std::move(other.impl_);
        return *this;
    }

    explicit operator bool() const noexcept {
        return impl_ != nullptr;
    }
	
    XAMP_DISABLE_COPY(MoveOnlyFunction)
	
private:
    /*
    * ImplBase is a virtual base class for ImplType.
    * It is used to avoid the need to know the type of the function
    * when calling Invoke().
    */
    struct XAMP_NO_VTABLE ImplBase {
        virtual ~ImplBase() = default;
        virtual void Invoke(const std::stop_token& stop_token) = 0;
    };

    ScopedPtr<ImplBase> impl_;

    template <typename Func>
    struct ImplType final : ImplBase {
        static_assert(std::is_invocable_v<Func, const std::stop_token&>,
            "Func must be callable with a const StopToken& argument.");

	    ImplType(Func&& f) noexcept(std::is_nothrow_move_assignable_v<Func>)
            : f_(std::forward<Func>(f)) {
        }

        void Invoke(const std::stop_token& stop_token) override {
            std::invoke(f_, stop_token);
        }
        Func f_;
    };
};

XAMP_BASE_NAMESPACE_END

