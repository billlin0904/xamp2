//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>

#include <base/base.h>

namespace xamp::base {

class XAMP_BASE_API StackTrace {
public:
    static constexpr size_t kMaxStackFrameSize = 62;
    using CaptureStackAddress = std::array<void*, kMaxStackFrameSize>;

    StackTrace() noexcept;   

    static void RegisterAbortHandler();

    static bool LoadSymbol();

#ifdef XAMP_OS_WIN
    std::string CaptureStack();
#endif
private:    
#ifdef XAMP_OS_WIN
    static LONG WINAPI AbortHandler(EXCEPTION_POINTERS* info);    
    void PrintStackTrace(EXCEPTION_POINTERS const * info);
    void WriteLog(size_t frame_count, std::ostream& ostr);
#else
    static void AbortHandler(int32_t signum);
    void PrintStackTrace();
#endif
    CaptureStackAddress addrlist_;
};

}


