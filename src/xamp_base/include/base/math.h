//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

namespace xamp::base {

inline constexpr float kPI = 3.14159265F;
inline constexpr float kLog2 = 1.44269504F;
inline constexpr float kLog10 = 0.434294482F;
inline constexpr float kSqrt2 = 1.41421356F;

inline size_t Log2(size_t value) noexcept {
    size_t result = 0;
    while ((1 << result) < value) {
        ++result;
    }
    return result;
}

}
