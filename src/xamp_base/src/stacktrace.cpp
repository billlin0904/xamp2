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
            auto* frame = addrlist[i];
            symbol_.clear();

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

class ExceptionHandler;

static std::vector<ExceptionHandler*> handlers_;
static std::atomic<int32_t> handler_index_{ 0 };
static FastMutex handlers_lock_;

class ExceptionHandler : public IExceptionHandler {
public:
    static constexpr int32_t kExceptionHandlerThreadInitialStackSize = 64 * 1024;
    static constexpr int32_t kHandleQueueSize = 64;

    ExceptionHandler()
		: is_shutdown_(false)
		, handle_queue_(kHandleQueueSize) {
        DWORD thread_id;
        handler_thread_.reset(::CreateThread(nullptr, // lpThreadAttributes
            kExceptionHandlerThreadInitialStackSize,
            ExceptionHandlerThreadMain,
            this,         // lpParameter
            0,            // dwCreationFlags
            &thread_id));
        handlers_.push_back(this);
    }

    virtual ~ExceptionHandler() {
        is_shutdown_ = true;
        handle_queue_.WakeupForShutdown();
    }

    void Send(EXCEPTION_POINTERS const* info) override {
        handle_queue_.Enqueue(info);
    }

private:
    void WriteCrashLog(EXCEPTION_POINTERS const* info) {
        const auto exception_code = info->ExceptionRecord->ExceptionCode;

        if ((exception_code & ERROR_SEVERITY_ERROR) != ERROR_SEVERITY_ERROR) {
            return;
        }
        if (exception_code & APPLICATION_ERROR_MASK) {
            return;
        }

        auto itr = kWellKnownExceptionCode.find(info->ExceptionRecord->ExceptionCode);
        if (itr != kWellKnownExceptionCode.end()) {
            XAMP_LOG_DEBUG("Caught signal 0x{:08x} {}.", info->ExceptionRecord->ExceptionCode, (*itr).second);
        }
        else {
            XAMP_LOG_DEBUG("Caught signal 0x{:08x}.", info->ExceptionRecord->ExceptionCode);
        }

        CaptureStackAddress addrlist;
        SYMBOL_LOADER.SetContext(info->ContextRecord);
        const auto frame_count = SYMBOL_LOADER.WalkStack(addrlist);
        SYMBOL_LOADER.WriteLog(ostr_, frame_count - 1, addrlist);
        XAMP_LOG_DEBUG(ostr_.str());
    }

    static DWORD ExceptionHandlerThreadMain(void* param) {
        auto* self = static_cast<ExceptionHandler*>(param);

        while (!self->is_shutdown_) {
            EXCEPTION_POINTERS const* pointer = nullptr;
            if (!self->handle_queue_.Dequeue(pointer)) {
	            continue;
            }

            if (!pointer) {
                break;
            }
            self->WriteCrashLog(pointer);
        }
        return 0;
    }

    std::atomic<bool> is_shutdown_;
    WinHandle handler_thread_;
    BoundedQueue<EXCEPTION_POINTERS const*> handle_queue_;
    std::ostringstream ostr_;
};

struct ExceptionHandlerGuard {
    ExceptionHandlerGuard() {
        handlers_lock_.lock();
        handler_ = handlers_[handler_index_];
    }

    ~ExceptionHandlerGuard() {
        handlers_lock_.unlock();
    }

    [[nodiscard]] ExceptionHandler* handler() const {
        return handler_;
    }
private:
    ExceptionHandler* handler_;
};

void StackTrace::PrintStackTrace(EXCEPTION_POINTERS const* info) {
    CaptureStackAddress addrlist;
    SYMBOL_LOADER.SetContext(info->ContextRecord);
    const auto frame_count = SYMBOL_LOADER.WalkStack(addrlist);
    std::ostringstream ostr;
    SYMBOL_LOADER.WriteLog(ostr, frame_count - 1, addrlist);
    XAMP_LOG_ERROR(ostr.str());
}

#else
void StackTrace::PrintStackTrace() {
    
}
#endif

StackTrace::StackTrace() noexcept = default;

bool StackTrace::LoadSymbol() {
#ifdef XAMP_OS_WIN
    return SYMBOL_LOADER.IsInit();
#else
    return true;
#endif
}

std::string StackTrace::CaptureStack() {
    CaptureStackAddress addrlist;
#ifdef XAMP_OS_WIN
    std::ostringstream ostr;
    auto frame_count = ::CaptureStackBackTrace(0, kMaxStackFrameSize, addrlist.data(), nullptr);
    SYMBOL_LOADER.WriteLog(ostr, frame_count - 1, addrlist);
    return ostr.str();
#else
    auto addrlen = ::backtrace(addrlist_.data(), static_cast<int32_t>(addrlist_.size()));
    if (addrlen == 0) {
        return "";
    }

    std::ostringstream ostr;
    auto symbollist = ::backtrace_symbols(addrlist_.data(), addrlen);
    for (auto i = 0; i < addrlen; i++) {
        ostr << symbollist[i] << "\r\n";
    }
    return ostr.str();
#endif
}

AlignPtr<IExceptionHandler> StackTrace::RegisterExceptionHandler() {
#ifdef XAMP_OS_WIN    
    (void) ::AddVectoredExceptionHandler(1, AbortHandler);
#else
    ::signal(SIGABRT, AbortHandler);
    ::signal(SIGSEGV, AbortHandler);
    ::signal(SIGILL, AbortHandler);
    ::signal(SIGFPE, AbortHandler);
#endif
    return MakeAlign<IExceptionHandler, ExceptionHandler>();
}

#ifdef XAMP_OS_WIN
LONG WINAPI StackTrace::AbortHandler(EXCEPTION_POINTERS* info) {
    ExceptionHandlerGuard gaurd;
    gaurd.handler()->Send(info);
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
    
}
#endif

}
