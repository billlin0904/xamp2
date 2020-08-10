#include <cstdio>
#include <csignal>
#include <vector>
#include <optional>
#include <map>

#include <base/logger.h>
#include <base/stl.h>
#include <base/stacktrace.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#include <dbghelp.h>
#else
#include <execinfo.h>
#endif

namespace xamp::base {

#ifdef XAMP_OS_WIN

inline constexpr DWORD kMsvcCppExceptionCode = 0xE06D7363;
inline constexpr DWORD kSetThreadNameExceptionCode = 0x406D1388;
inline constexpr DWORD kClrDbgNotificationExceptionCode = 0x04242420;
inline constexpr DWORD kIgoneDebugOutputStringExceptionCode = DBG_PRINTEXCEPTION_C;
inline constexpr DWORD kIgoneDebugOutputWideStringExceptionCode = DBG_PRINTEXCEPTION_WIDE_C;

inline constexpr std::array<DWORD, 8> kIgoneExceptionCode{
    kIgoneDebugOutputStringExceptionCode,
    kIgoneDebugOutputWideStringExceptionCode,
    kSetThreadNameExceptionCode,
    kClrDbgNotificationExceptionCode,
    0x000006BA,    
    0xE0000001,
    0x000006A6,
    0x800706B5,
};

#define DECLARE_EXCEPTION_CODE(Code) { Code, #Code },

static RobinHoodHashMap<DWORD, std::string_view> const & GetWellKnownExceptionCode() {
    static const RobinHoodHashMap<DWORD, std::string_view> WellKnownExceptionCode = {
    DECLARE_EXCEPTION_CODE(EXCEPTION_ACCESS_VIOLATION)
    DECLARE_EXCEPTION_CODE(EXCEPTION_BREAKPOINT)
    DECLARE_EXCEPTION_CODE(EXCEPTION_SINGLE_STEP)
    DECLARE_EXCEPTION_CODE(EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
    DECLARE_EXCEPTION_CODE(EXCEPTION_FLT_DENORMAL_OPERAND)
    DECLARE_EXCEPTION_CODE(EXCEPTION_FLT_DIVIDE_BY_ZERO)
    DECLARE_EXCEPTION_CODE(EXCEPTION_FLT_INEXACT_RESULT)
    DECLARE_EXCEPTION_CODE(EXCEPTION_FLT_INVALID_OPERATION)
    DECLARE_EXCEPTION_CODE(EXCEPTION_FLT_OVERFLOW)
    DECLARE_EXCEPTION_CODE(EXCEPTION_FLT_STACK_CHECK)
    DECLARE_EXCEPTION_CODE(EXCEPTION_FLT_UNDERFLOW)
    DECLARE_EXCEPTION_CODE(EXCEPTION_INT_DIVIDE_BY_ZERO)
    DECLARE_EXCEPTION_CODE(EXCEPTION_INT_OVERFLOW)
    DECLARE_EXCEPTION_CODE(EXCEPTION_PRIV_INSTRUCTION)
    DECLARE_EXCEPTION_CODE(EXCEPTION_IN_PAGE_ERROR)
    DECLARE_EXCEPTION_CODE(EXCEPTION_ILLEGAL_INSTRUCTION)
    DECLARE_EXCEPTION_CODE(EXCEPTION_NONCONTINUABLE_EXCEPTION)
    DECLARE_EXCEPTION_CODE(EXCEPTION_STACK_OVERFLOW)
    DECLARE_EXCEPTION_CODE(EXCEPTION_INVALID_DISPOSITION)
    DECLARE_EXCEPTION_CODE(EXCEPTION_GUARD_PAGE)
    DECLARE_EXCEPTION_CODE(EXCEPTION_INVALID_HANDLE)
    DECLARE_EXCEPTION_CODE(kMsvcCppExceptionCode)
    };

    return WellKnownExceptionCode;
}

class SymLoader {
public:
    static SymLoader& Instance() {
        static SymLoader loader;
        return loader;
    }

    const WinHandle & GetProcess() const noexcept {
		return process_;
	}

	~SymLoader() {
        ::SymCleanup(process_.get());
	}
private:
    SymLoader() {
        process_.reset(::GetCurrentProcess());

        ::SymSetOptions(SYMOPT_DEFERRED_LOADS |
            SYMOPT_UNDNAME |
            SYMOPT_LOAD_LINES);

        ::SymInitialize(process_.get(), nullptr, TRUE);
    }

	WinHandle process_;
};

static size_t WalkStack(CONTEXT const* context, CaptureStackAddress& addrlist) {
    CONTEXT integer_control_context = *context;
    integer_control_context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;

    STACKFRAME64 stack_frame{};

    stack_frame.AddrPC.Offset = context->Rip;
    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrStack.Offset = context->Rsp;
    stack_frame.AddrStack.Mode = AddrModeFlat;

    stack_frame.AddrFrame.Offset = context->Rbp;
    stack_frame.AddrFrame.Mode = AddrModeFlat;

    size_t frame_count = 0;
    const FileHandle thread(::GetCurrentThread());

    for (auto i = 0; i < addrlist.size(); ++i) {
        const auto result = ::StackWalk64(IMAGE_FILE_MACHINE_IA64,
            SymLoader::Instance().GetProcess().get(),
            thread.get(),
            &stack_frame,
            &integer_control_context,
            nullptr,
            ::SymFunctionTableAccess64,
            ::SymGetModuleBase64,
            nullptr);
        if (!result) {
            break;
        }
        if (stack_frame.AddrFrame.Offset == 0) {
            break;
        }
        addrlist[i] = reinterpret_cast<void*>(stack_frame.AddrPC.Offset);
        ++frame_count;
    }
    return frame_count;
}

void StackTrace::WriteLog(size_t frame_count) {
    std::vector<uint8_t> symbol(sizeof(SYMBOL_INFO) + sizeof(wchar_t) * MAX_SYM_NAME);

    for (size_t i = 0; i < frame_count; ++i) {
        auto frame = addrlist_[i];
        symbol.clear();

        auto* const symbol_info = reinterpret_cast<SYMBOL_INFO*>(symbol.data());

        symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol_info->MaxNameLen = 256;
        symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol_info->MaxNameLen = 256;

        DWORD64 displacement = 0;
        const auto has_symbol = ::SymFromAddr(SymLoader::Instance().GetProcess().get(),
            reinterpret_cast<DWORD64>(frame),
            &displacement,
            symbol_info);

        if (has_symbol) {
            DWORD line_displacement = 0;

            IMAGEHLP_LINE64 line{};
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

            const auto has_line = ::SymGetLineFromAddr64(SymLoader::Instance().GetProcess().get(),
                reinterpret_cast<DWORD64>(frame),
                &line_displacement,
                &line);

            if (has_line) {
                XAMP_LOG_DEBUG("0x{:08x} {:08x} {}:{}", reinterpret_cast<DWORD64>(frame), displacement, line.FileName, line.LineNumber);
            }
            else {
                XAMP_LOG_DEBUG("0x{:08x} {:08x}", reinterpret_cast<DWORD64>(frame), displacement);
            }
        }
        else {
            XAMP_LOG_DEBUG("0x{:08d} (No symbol)", reinterpret_cast<DWORD64>(frame));
        }
    }
}

void StackTrace::PrintStackTrace(EXCEPTION_POINTERS const* info) {
    if (std::find(kIgoneExceptionCode.begin(),
        kIgoneExceptionCode.end(),
        info->ExceptionRecord->ExceptionCode) != kIgoneExceptionCode.end()) {
        XAMP_LOG_DEBUG("Igone exception code 0x{:08x}.", info->ExceptionRecord->ExceptionCode);
        return;
    }

    if (info->ExceptionRecord->ExceptionCode == kMsvcCppExceptionCode) {
        XAMP_LOG_DEBUG("Uncaught std::exception!");
        return;
    }

    auto itr = GetWellKnownExceptionCode().find(info->ExceptionRecord->ExceptionCode);
    if (itr != GetWellKnownExceptionCode().end()) {
        XAMP_LOG_DEBUG("Caught signal 0x{:08x} {}.", info->ExceptionRecord->ExceptionCode, (*itr).second);
    }
    else {
        XAMP_LOG_DEBUG("Caught signal 0x{:08x}.", info->ExceptionRecord->ExceptionCode);
    }

    auto frame_count = WalkStack(info->ContextRecord, addrlist_);
    WriteLog(frame_count);
    std::exit(0);
}

#else
void StackTrace::PrintStackTrace() {
    auto addrlen = ::backtrace(addrlist_.data(), static_cast<int32_t>(addrlist_.size()));
    if (addrlen == 0) {
        return;
    }

    auto symbollist = ::backtrace_symbols(addrlist_.data(), addrlen);
    for (auto i = 4; i < addrlen; i++) {
        XAMP_LOG_DEBUG("{}", symbollist[i]);
    }
}
#endif

StackTrace::StackTrace() noexcept {
    addrlist_.fill(nullptr);
}

void StackTrace::RegisterAbortHandler() {
#ifdef XAMP_OS_WIN
    SymLoader::Instance();
    (void) ::AddVectoredExceptionHandler(1, AbortHandler);
#else
    ::signal(SIGABRT, AbortHandler);
    ::signal(SIGSEGV, AbortHandler);
    ::signal(SIGILL, AbortHandler);
    ::signal(SIGFPE, AbortHandler);
#endif
}

#ifdef XAMP_OS_WIN
LONG WINAPI StackTrace::AbortHandler(EXCEPTION_POINTERS* info) {
    StackTrace trace;
    trace.PrintStackTrace(info);
    return EXCEPTION_CONTINUE_SEARCH;
}
#else
void StackTrace::AbortHandler(int32_t signum) {
    const char* name = "";

    switch (signum) {
    case SIGABRT:
        name = "SIGABRT";
        break;
    case SIGSEGV:
        name = "SIGSEGV";
        break;
    case SIGBUS:
        name = "SIGBUS";
        break;
    case SIGILL:
        name = "SIGILL";
        break;
    case SIGFPE:
        name = "SIGFPE";
        break;
    }

    XAMP_LOG_DEBUG("Caught signal {} {}", signum, !name ? "" : name);

    StackTrace trace;
    trace.PrintStackTrace();
}
#endif

}
