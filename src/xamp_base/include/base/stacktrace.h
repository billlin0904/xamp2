//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>

#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#endif

namespace xamp::base {

class XAMP_BASE_API StackTrace {
public:
    static constexpr size_t kMaxStackFrameSize = 62;
    using CaptureStackAddress = std::array<void*, kMaxStackFrameSize>;

    StackTrace() noexcept;   

    static void RegisterAbortHandler();

    static bool LoadSymbol();

    std::string CaptureStack();

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


