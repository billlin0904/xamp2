//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cmath>
#include <base/base.h>

namespace xamp::base {

template <typename T>
T Round(T a) {
    static_assert(std::is_floating_point_v<T>, "Round<T>: T must be floating point");
    return (a > 0) ? ::floor(a + static_cast<T>(0.5)) : ::ceil(a - static_cast<T>(0.5));
}

template <typename T>
T Round(T a, int32_t places) {
    static_assert(std::is_floating_point_v<T>, "Round<T>: T must be floating point");
    const T shift = pow(static_cast<T>(10.0), places);
    return Round(a * shift) / shift;
}

}

