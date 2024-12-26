//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cassert>
#include <base/base.h>
#include <base/platform.h>

#if defined(_DEBUG) || !defined(NDEBUG)
#define XAMP_ASSERT(expr) (void)(                              \
            (!!(expr)) ||                                      \
            (xamp::base::Assert(#expr, __FILE__, (unsigned)(__LINE__)), 0) \
        )
#else
#define XAMP_ASSERT(expr) ((void)0)
#endif

namespace xamp {
    namespace details {
        inline void terminate() noexcept {
            using std::terminate;
            std::terminate();
        }
    }
}

#define XAMP_CONTRACT_CHECK(type, cond) \
	((cond) ? static_cast<void>(0) : xamp::details::terminate())

#define XAMP_EXPECTS(cond) XAMP_CONTRACT_CHECK("Precondition", cond)
#define XAMP_ENSURES(cond) XAMP_CONTRACT_CHECK("Postcondition", cond)
