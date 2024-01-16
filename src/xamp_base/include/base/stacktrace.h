//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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

    std::string CaptureStack();
};

XAMP_BASE_NAMESPACE_END