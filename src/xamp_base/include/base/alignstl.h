//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <deque>

#include <base/alignallocator.h>

namespace xamp::base {

template <typename T>
using Vector = std::vector<T, AlignedAllocator<T>>;

template <typename T>
using Queue = std::deque<T, AlignedAllocator<T>>;

}

