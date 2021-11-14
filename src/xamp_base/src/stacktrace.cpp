#include <cstdio>
#include <csignal>
#include <vector>
#include <optional>
#include <sstream>
#include <filesystem>

#include <base/logger.h>
#include <base/stl.h>
#include <base/singleton.h>
#include <base/stacktrace.h>
#include <base/str_utilts.h>
#include <base/align_ptr.h>
#include <base/bounded_queue.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#include <dbghelp.h>
#else
#include <execinfo.h>
#endif

namespace xamp::base {

#ifdef XAMP_OS_WIN

#define DECLARE_EXCEPTION_CODE(Code) { Code, #Code },

static const HashMap<DWORD, std::string_view> kWellKnownExceptionCode = {
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

static std::string GetFileName(std::filesystem::path const& path) {
    return String::ToUtf8String(path.filename());
}

class SymLoader {
public:
	[[nodiscard]] const WinHandle & GetProcess() const noexcept {
		return process_;
	}

    SymLoader() {
        symbol_.resize(sizeof(SYMBOL_INFO) + sizeof(wchar_t) * MAX_SYM_NAME);
        process_.reset(::GetCurrentProcess());
        thread_.reset(::GetCurrentThread());

        ::SymSetOptions(SYMOPT_DEFERRED_LOADS |
            SYMOPT_UNDNAME |
            SYMOPT_LOAD_LINES /*| SYMOPT_DEBUG*/);

        init_state_ = ::SymInitialize(process_.get(),
            nullptr, 
            TRUE);

        if (init_state_) {
            wchar_t path[MAX_PATH] = { 0 };
            ::GetModuleFileNameW(nullptr, path, MAX_PATH);
            Path excute_file_path(path);
            auto parent_path = excute_file_path.parent_path();
            ::SymSetSearchPathW(process_.get(), parent_path.c_str());
        }
    }

    [[nodiscard]] bool IsInit() const noexcept {
        return init_state_;
    }

	~SymLoader() {
        ::SymCleanup(process_.get());
	}

    void SetContext(CONTEXT const* context) {
        MemoryCopy(&context_, context, sizeof(CONTEXT));
    }

    size_t WalkStack(CaptureStackAddress& addrlist) noexcept {
        addrlist.fill(0);

        CONTEXT integer_control_context = context_;
        integer_control_context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;

        STACKFRAME64 stack_frame{};

        stack_frame.AddrPC.Offset = context_.Rip;
        stack_frame.AddrPC.Mode = AddrModeFlat;
        stack_frame.AddrStack.Offset = context_.Rsp;
        stack_frame.AddrStack.Mode = AddrModeFlat;

        stack_frame.AddrFrame.Offset = context_.Rbp;
        stack_frame.AddrFrame.Mode = AddrModeFlat;

        size_t frame_count = 0;

        for (auto& address : addrlist) {
            const auto result = ::StackWalk64(IMAGE_FILE_MACHINE_IA64,
                process_.get(),
                thread_.get(),
                &stack_frame,
                &integer_control_context,
                nullptr,
                ::SymFunctionTableAccess64,
                ::SymGetModuleBase64,
                nullptr);
            if (!result) {
                address = nullptr;
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

    void WriteLog(std::ostringstream &ostr, size_t frame_count, CaptureStackAddress& addrlist) {
        ostr << "\r\nstack backtrace:\r\n";

        frame_count = (std::min)(addrlist.size(), frame_count);

        for (size_t i = 0; i < frame_count; ++i) {
            MemorySet(symbol_.data(), 0, symbol_.size());
            auto* frame = addrlist[i];
            auto* const symbol_info = reinterpret_cast<SYMBOL_INFO*>(symbol_.data());

            symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol_info->MaxNameLen = 256;
            symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol_info->MaxNameLen = 256;

            DWORD64 displacement = 0;
            const auto has_symbol = ::SymFromAddr(process_.get(),
                reinterpret_cast<DWORD64>(frame),
                &displacement,
                symbol_info);

            if (has_symbol) {
                DWORD line_displacement = 0;

                IMAGEHLP_LINE64 line{};
                line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

                const auto has_line = ::SymGetLineFromAddr64(process_.get(),
                    reinterpret_cast<DWORD64>(frame),
                    &line_displacement,
                    &line);

                if (has_line) {
                    ostr << std::setw(2) << i << ":" << std::setw(8) << "0x" << std::hex << reinterpret_cast<DWORD64>(frame) << " "
                        << symbol_info->Name
                        << " "
                        << GetFileName(line.FileName) << ":" << std::dec << line.LineNumber << "\r\n";
                }
                else {
                    ostr << std::setw(2) << i << ":" << std::setw(8) << "0x" << std::hex << reinterpret_cast<DWORD64>(frame) << " "
                        << symbol_info->Name
                        << " offset " << std::dec << displacement << "\r\n";
                }
            }
            else {
                ostr << std::setw(2) << i << ":" << std::setw(8) << "0x" << reinterpret_cast<DWORD64>(frame) << " <unknown>" << "\r\n";
            }
        }
    }

private:
    bool init_state_;
	WinHandle process_;
    WinHandle thread_;
    CONTEXT context_;
    std::vector<uint8_t> symbol_;
};

#define SYMBOL_LOADER Singleton<SymLoader>::GetInstance()

#else

static bool HaveSiginfo(int signum) {
    struct sigaction old_action, new_action;
    MemorySet(&new_action, 0, sizeof(new_action));

    new_action.sa_handler = SIG_DFL;
    new_action.sa_flags = SA_RESTART;
    sigemptyset(&new_action.sa_mask);

    if (::sigaction(signum, &new_action, &old_action) < 0) {
        return false;
    }

    bool result = (old_action.sa_flags & SA_SIGINFO) != 0;
    if (::sigaction(signum, &old_action, nullptr) == -1) {
        XAMP_LOG_ERROR("Restore failed in test for SA_SIGINFO: {}", strerror(errno));
    }

    return result;
}

static void LogCrashSignal(int signum, const siginfo_t* info) {
    const char* signal_name = "???";
    bool has_address = false;

    switch (signum) {
    case SIGABRT:
        signal_name = "SIGABRT";
        break;
    case SIGBUS:
        signal_name = "SIGBUS";
        has_address = true;
        break;
    case SIGFPE:
        signal_name = "SIGFPE";
        has_address = true;
        break;
    case SIGILL:
        signal_name = "SIGILL";
        has_address = true;
        break;
    case SIGSEGV:
        signal_name = "SIGSEGV";
        has_address = true;
        break;
    case SIGTRAP:
        signal_name = "SIGTRAP";
        break;
    }

    XAMP_LOG_ERROR("Fatal signal {} ({}){}{}",
                   signum, signal_name, info->si_code, info->si_addr);
    StackTrace trace;
    XAMP_LOG_ERROR(trace.CaptureStack());
}

 [[noreturn]] static void CrashSignalHandler(int signal_number, siginfo_t* info, void*) {
    if (!HaveSiginfo(signal_number)) {
        info = nullptr;
    }

    LogCrashSignal(signal_number, info);

    // Reset signal to SIG_DFL
    ::signal(signal_number, SIG_DFL);

    std::exit(0);
}

static void InstallCrashSignal() {
    struct sigaction action;
    MemorySet(&action, 0, sizeof(action));

    sigemptyset(&action.sa_mask);
    action.sa_sigaction = CrashSignalHandler;
    action.sa_flags = SA_RESTART | SA_SIGINFO;
    action.sa_flags |= SA_ONSTACK;

    ::sigaction(SIGABRT, &action, nullptr);
    ::sigaction(SIGBUS, &action, nullptr);
    ::sigaction(SIGFPE, &action, nullptr);
    ::sigaction(SIGILL, &action, nullptr);
    ::sigaction(SIGPIPE, &action, nullptr);
    ::sigaction(SIGSEGV, &action, nullptr);
    ::sigaction(SIGTRAP, &action, nullptr);
}

#endif

StackTrace::StackTrace() noexcept = default;

bool StackTrace::LoadSymbol() {
#ifdef XAMP_OS_WIN
    return SYMBOL_LOADER.IsInit();
#else
    InstallCrashSignal();
    return true;
#endif
}

std::string StackTrace::CaptureStack() {
    CaptureStackAddress addrlist;
#ifdef XAMP_OS_WIN
    addrlist.fill(0);
    std::ostringstream ostr;
    auto frame_count = ::CaptureStackBackTrace(0, kMaxStackFrameSize, addrlist.data(), nullptr);
    SYMBOL_LOADER.WriteLog(ostr, frame_count - 1, addrlist);
    return ostr.str();
#else
    auto addrlen = ::backtrace(addrlist.data(), static_cast<int32_t>(addrlist.size()));
    if (addrlen == 0) {
        return "";
    }

    std::ostringstream ostr;
    auto symbollist = ::backtrace_symbols(addrlist.data(), addrlen);
    for (auto i = 0; i < addrlen; i++) {
        ostr << symbollist[i] << "\r\n";
    }
    return ostr.str();
#endif
}

}
