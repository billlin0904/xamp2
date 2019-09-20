//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <memory>

namespace xamp::base {

template <typename T>
using BufferPtr = std::unique_ptr<T[]>;

}

