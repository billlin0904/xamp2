//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>

#include <base/base.h>

namespace xamp::base {

static const size_t MaxStackFrameSize = 62;
using CaptureStackAddress = std::array<void*, MaxStackFrameSize>;

class StackTrace {
public:
    StackTrace();

    static void RegisterSignal();

private:   
#ifdef _WIN32
    static LONG WINAPI AbortHandler(EXCEPTION_POINTERS* info);
    void WriteLog(size_t frame_count);
    void PrintStackTrace(EXCEPTION_POINTERS* info);
#else
    static void AbortHandler(int32_t signum);
    void PrintStackTrace();
#endif
    CaptureStackAddress addrlist_;
};

}


