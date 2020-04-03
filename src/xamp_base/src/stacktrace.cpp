#include <cstdio>
#include <csignal>
#include <vector>
#include <optional>
#include <map>

#ifdef _WIN32
#include <base/windows_handle.h>
#include <dbghelp.h>
#else
#include <execinfo.h>
#endif

#include <base/logger.h>
#include <base/alignstl.h>
#include <base/stacktrace.h>

namespace xamp::base {

#ifdef _WIN32

constexpr DWORD MSVC_CPP_EXCEPTION_CODE = 0xE06D7363;

#define DECLARE_EXCEPTION_CODE(Code) { Code, #Code },

const RobinHoodHashMap<DWORD, std::string_view> WellKnownExceptionCode = {
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
    DECLARE_EXCEPTION_CODE(MSVC_CPP_EXCEPTION_CODE)
};

class SymLoader {
public:
    static SymLoader& Instance() {
        static SymLoader loader;
        return loader;
    }

	XAMP_DISABLE_COPY(SymLoader)

    const WinHandle & GetProcess() const {
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

static std::optional<const std::exception*> FindException(DWORD64 addr) {
    auto cpp_exception = reinterpret_cast<const std::exception*>(addr);

    __try {
        if (cpp_exception) {
            cpp_exception->what();
        }
        return cpp_exception;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return std::nullopt;
    }
}

static size_t WalkStack(const CONTEXT* context, CaptureStackAddress& addrlist) {
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
        const auto result = StackWalk64(IMAGE_FILE_MACHINE_IA64,
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

        const auto symbol_info = reinterpret_cast<SYMBOL_INFO*>(symbol.data());

        symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol_info->MaxNameLen = 256;
        symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol_info->MaxNameLen = 256;

        DWORD64 displacement = 0;
        const auto has_symbol = ::SymFromAddr(SymLoader::Instance().GetProcess().get(),
            (DWORD64)frame,
            &displacement,
            symbol_info);

        DWORD line_displacement = 0;
        IMAGEHLP_LINE64 line{};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        const auto has_line = ::SymGetLineFromAddr64(SymLoader::Instance().GetProcess().get(),
            (DWORD64)frame,
            &line_displacement,
            &line);

        if (has_symbol) {
            if (has_line) {
                XAMP_LOG_DEBUG("0x{:08d} {:08d} {}:{}", (DWORD64)frame, displacement, line.FileName, line.LineNumber);
            }
            else {
                XAMP_LOG_DEBUG("0x{:08d} {:08d}", (DWORD64)frame, displacement);
            }
        }
        else {
            XAMP_LOG_DEBUG("0x{:08d} (No symbol)", (DWORD64)frame);
        }
    }
}

void StackTrace::PrintStackTrace(EXCEPTION_POINTERS* info) {
    //if (info->ExceptionRecord->ExceptionCode == MSVC_CPP_EXCEPTION_CODE) {
    //    return;
    //}

    auto itr = WellKnownExceptionCode.find(info->ExceptionRecord->ExceptionCode);
    if (itr != WellKnownExceptionCode.end()) {
        XAMP_LOG_DEBUG("Caught signal {} {}", info->ExceptionRecord->ExceptionCode, (*itr).second);
    }
    else {
        XAMP_LOG_DEBUG("Caught signal {}", info->ExceptionRecord->ExceptionCode);
    }

    auto frame_count = WalkStack(info->ContextRecord, addrlist_);
    WriteLog(frame_count);
}

#else
void StackTrace::PrintStackTrace() {
    auto addrlen = backtrace(addrlist.data(), static_cast<int32_t>(addrlist.size()));
    if (addrlen == 0) {
        return;
    }

    auto symbollist = backtrace_symbols(addrlist.data(), addrlen);
    for (auto i = 4; i < addrlen; i++) {
        XAMP_LOG_DEBUG("{}", symbollist[i]);
    }
}
#endif

StackTrace::StackTrace() {
    addrlist_.fill(nullptr);
}

void StackTrace::RegisterSignal() {
#ifdef _WIN32
    (void) ::AddVectoredExceptionHandler(1, AbortHandler);
#else
    signal(SIGABRT, AbortHandler);
    signal(SIGSEGV, AbortHandler);
    signal(SIGILL, AbortHandler);
    signal(SIGFPE, AbortHandler);
#endif
}

#ifdef _WIN32
LONG WINAPI StackTrace::AbortHandler(EXCEPTION_POINTERS* info) {
    StackTrace trace;
    trace.PrintStackTrace(info);
    return EXCEPTION_CONTINUE_EXECUTION;
}
#else
void StackTrace::AbortHandler(int32_t signum) {
    const char* name = nullptr;

    switch (signum) {
    case SIGABRT: name = "SIGABRT";  break;
    case SIGSEGV: name = "SIGSEGV";  break;

    case SIGBUS:  name = "SIGBUS";   break;
    case SIGILL:  name = "SIGILL";   break;
    case SIGFPE:  name = "SIGFPE";   break;
    }

    XAMP_LOG_DEBUG("Caught signal {} {}", signum, !name ? "" : name);

    StackTrace trace;
    trace.PrintStackTrace();
}
#endif

}
