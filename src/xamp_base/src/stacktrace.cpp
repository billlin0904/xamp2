#include <cstdio>
#include <csignal>
#include <vector>
#include <optional>
#include <map>
#include <sstream>

#include <base/logger.h>
#include <base/stl.h>
#include <base/singleton.h>
#include <base/stacktrace.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#include <dbghelp.h>
#else
#include <execinfo.h>
#endif

namespace xamp::base {

#ifdef XAMP_OS_WIN

#define DECLARE_EXCEPTION_CODE(Code) { Code, #Code },

static HashMap<DWORD, std::string_view> const & GetWellKnownExceptionCode() {
    static const HashMap<DWORD, std::string_view> WellKnownExceptionCode = {
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
    };

    return WellKnownExceptionCode;
}

class SymLoader {
public:
    const WinHandle & GetProcess() const noexcept {
		return process_;
	}

    SymLoader() {
        process_.reset(::GetCurrentProcess());

        ::SymSetOptions(SYMOPT_DEFERRED_LOADS |
            SYMOPT_UNDNAME |
            SYMOPT_LOAD_LINES);

        init_state_ = ::SymInitialize(process_.get(), nullptr, TRUE);
    }

    bool IsInit() const noexcept {
        return init_state_;
    }

	~SymLoader() {
        ::SymCleanup(process_.get());
	}

private:
    bool init_state_;
	WinHandle process_;
};

static size_t WalkStack(CONTEXT const* context, StackTrace::CaptureStackAddress& addrlist) noexcept {
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
    const WinHandle thread(::GetCurrentThread());

    for (auto &address : addrlist) {
        const auto result = ::StackWalk64(IMAGE_FILE_MACHINE_IA64,
            Singleton<SymLoader>::GetInstance().GetProcess().get(),
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
        address = reinterpret_cast<void*>(stack_frame.AddrPC.Offset);
        ++frame_count;
    }
    return frame_count;
}

void StackTrace::WriteLog(size_t frame_count, std::ostream& ostr) {
    std::vector<uint8_t> symbol(sizeof(SYMBOL_INFO) + sizeof(wchar_t) * MAX_SYM_NAME);
    auto current_process = Singleton<SymLoader>::GetInstance().GetProcess().get();

    ostr << "\r\n";    

    for (size_t i = 0; i < frame_count; ++i) {
        auto frame = addrlist_[i];
        symbol.clear();

        auto* const symbol_info = reinterpret_cast<SYMBOL_INFO*>(symbol.data());

        symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol_info->MaxNameLen = 256;
        symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol_info->MaxNameLen = 256;

        DWORD64 displacement = 0;
        const auto has_symbol = ::SymFromAddr(current_process,
            reinterpret_cast<DWORD64>(frame),
            &displacement,
            symbol_info);

        if (has_symbol) {
            DWORD line_displacement = 0;

            IMAGEHLP_LINE64 line{};
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

            const auto has_line = ::SymGetLineFromAddr64(current_process,
                reinterpret_cast<DWORD64>(frame),
                &line_displacement,
                &line);

            if (has_line) {                
                ostr << "0x" << std::hex << reinterpret_cast<DWORD64>(frame) << " " 
                    << std::dec << displacement << " "
                    << line.FileName << ":" << line.LineNumber << "\r\n";
            }
            else {                
                ostr << "0x" << std::hex << reinterpret_cast<DWORD64>(frame) << " "
                    << std::dec << displacement << "\r\n";
            }
        }
        else {
            ostr << "0x" << reinterpret_cast<DWORD64>(frame) << "(No symbol)" << "\r\n";
        }
    }
}

void StackTrace::PrintStackTrace(EXCEPTION_POINTERS const* info) {
    auto exceptionCode = info->ExceptionRecord->ExceptionCode;

    if ((exceptionCode & ERROR_SEVERITY_ERROR) != ERROR_SEVERITY_ERROR) {
        return;
    }
    if (exceptionCode & APPLICATION_ERROR_MASK) {
        return;
    }

    auto itr = GetWellKnownExceptionCode().find(info->ExceptionRecord->ExceptionCode);
    if (itr != GetWellKnownExceptionCode().end()) {
        XAMP_LOG_DEBUG("Caught signal 0x{:08x} {}.", info->ExceptionRecord->ExceptionCode, (*itr).second);
    }
    else {
        XAMP_LOG_DEBUG("Caught signal 0x{:08x}.", info->ExceptionRecord->ExceptionCode);
    }

    std::ostringstream ostr;
    auto frame_count = WalkStack(info->ContextRecord, addrlist_);
    WriteLog(frame_count - 1, ostr);
    XAMP_LOG_DEBUG(ostr.str());
    std::exit(-1);
}

#else
void StackTrace::PrintStackTrace() {
    XAMP_LOG_DEBUG("{}", CaptureStack());
}
#endif

StackTrace::StackTrace() noexcept {
    addrlist_.fill(nullptr);
}

bool StackTrace::LoadSymbol() {
#ifdef XAMP_OS_WIN
    return Singleton<SymLoader>::GetInstance().IsInit();
#else
    return true;
#endif
}

std::string StackTrace::CaptureStack() {
#ifdef XAMP_OS_WIN
    std::ostringstream ostr;
    auto frame_count = ::CaptureStackBackTrace(0, kMaxStackFrameSize, addrlist_.data(), nullptr);
    WriteLog(frame_count - 1, ostr);
    return ostr.str();
#else
    auto addrlen = ::backtrace(addrlist_.data(), static_cast<int32_t>(addrlist_.size()));
    if (addrlen == 0) {
        return "";
    }

    std::ostringstream ostr;
    auto symbollist = ::backtrace_symbols(addrlist_.data(), addrlen);
    for (auto i = 4; i < addrlen; i++) {
        ostr << symbollist[i];
    }
    return ostr.str();
#endif
}

void StackTrace::RegisterAbortHandler() {
#ifdef XAMP_OS_WIN    
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
