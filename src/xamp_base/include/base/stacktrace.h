//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>

namespace xamp::base {

static constexpr size_t kMaxStackFrameSize = 62;
using CaptureStackAddress = std::array<void*, kMaxStackFrameSize>;

class XAMP_BASE_API StackTrace {
public:
    StackTrace() noexcept;   

    static bool LoadSymbol();

    std::string CaptureStack();
};

}


