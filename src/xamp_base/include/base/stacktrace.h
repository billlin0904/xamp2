//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#include <array>
#include <string>

XAMP_BASE_NAMESPACE_BEGIN

static constexpr size_t kMaxStackFrameSize = 62;
using CaptureStackAddress = std::array<void*, kMaxStackFrameSize>;

class XAMP_BASE_API StackTrace final {
public:
    StackTrace() noexcept;   

    static bool LoadSymbol();

    std::string CaptureStack(uint32_t skip = 5);
};

XAMP_BASE_NAMESPACE_END