//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <vector>

#include <base/align_ptr.h>
#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#endif

namespace xamp::base {

static constexpr size_t kMaxStackFrameSize = 62;
using CaptureStackAddress = std::array<void*, kMaxStackFrameSize>;

#ifdef XAMP_OS_WIN
class XAMP_BASE_API XAMP_NO_VTABLE IExceptionHandler {
public:
    XAMP_BASE_CLASS(IExceptionHandler)

	virtual void Send(EXCEPTION_POINTERS const* info) = 0;
protected:
    IExceptionHandler() = default;
};
#endif

class XAMP_BASE_API StackTrace {
public:
    StackTrace() noexcept;   

    static AlignPtr<IExceptionHandler> RegisterExceptionHandler();

    static bool LoadSymbol();

    std::string CaptureStack();

private:    
#ifdef XAMP_OS_WIN
    static LONG WINAPI AbortHandler(EXCEPTION_POINTERS* info);
    void PrintStackTrace(EXCEPTION_POINTERS const* info);
#else
    static void AbortHandler(int32_t signum);
    void PrintStackTrace();
#endif
};

}


