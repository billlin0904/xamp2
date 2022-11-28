//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <algorithm>
#include <future>
#include <base/base.h>
#include <base/align_ptr.h>

namespace xamp::base {

class MoveableFunction final {
public:
    template <typename Func>
    MoveableFunction(Func&& f)
        : impl_(MakeAlign<ImplBase, ImplType<Func>>(std::forward<Func>(f))) {
    }

    XAMP_ALWAYS_INLINE void operator()() const {
	    impl_->Invoke();
    }

    MoveableFunction() = default;
	
    MoveableFunction(MoveableFunction&& other) noexcept
		: impl_(std::move(other.impl_)) {	    
    }
	
    MoveableFunction& operator=(MoveableFunction&& other) noexcept {
        impl_ = std::move(other.impl_);
        return *this;
    }
	
    XAMP_DISABLE_COPY(MoveableFunction)
	
private:
    struct XAMP_NO_VTABLE ImplBase {
        virtual ~ImplBase() = default;
        virtual void Invoke() = 0;
    };

    AlignPtr<ImplBase> impl_;

    template <typename Func>
    struct ImplType final : ImplBase {
	    ImplType(Func&& f)
            : f_(std::forward<Func>(f)) {
        }

        XAMP_ALWAYS_INLINE void Invoke() override {
            f_();
        }
        Func f_;
    };
};

}

