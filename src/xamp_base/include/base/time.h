//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>

#include <base/base.h>

namespace xamp::base {

template <typename Resolution = std::chrono::microseconds>
uint64_t GetUnixTime() {
    return std::chrono::duration_cast<Resolution>(
               std::chrono::system_clock::now().time_since_epoch()).count();
}

}
