#include <base/crashhandler.h>
#include <base/dll.h>
#include <base/memory.h>
#include <base/logger.h>
#include <base/stacktrace.h>
#include <base/stl.h>
#include <base/fastmutex.h>
#include <base/platfrom_handle.h>

#ifdef XAMP_OS_WIN
#include <new.h>
#include <dbghelp.h>
#else
#include <signal.h>
#include <execinfo.h>
#endif

#include <csignal>
#include <mutex>

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN
#define DECLARE_EXCEPTION_CODE(Code) { Code, #Code },

// What is a First Chance Exception?  
// https://docs.microsoft.com/en-us/archive/blogs/davidklinems/what-is-a-first-chance-exception
#define EXCEPTION_FIRST_CHANCE                  0X000004242420
#define EXCEPTION_RPC_SERVER_NOT_UNAVAILABLE    0X0000000006BA
#define EXCEPTION_MSVC_CPP                      0X0000E06D7363

static const HashMap<DWORD, std::string_view> kIgnoreExceptionCode = {
    DECLARE_EXCEPTION_CODE(EXCEPTION_RPC_SERVER_NOT_UNAVAILABLE)
    DECLARE_EXCEPTION_CODE(EXCEPTION_FIRST_CHANCE)
    DECLARE_EXCEPTION_CODE(DBG_PRINTEXCEPTION_C)
    DECLARE_EXCEPTION_CODE(DBG_PRINTEXCEPTION_WIDE_C)
    DECLARE_EXCEPTION_CODE(EXCEPTION_MSVC_CPP)
};

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

struct ExceptionPointer : EXCEPTION_POINTERS {
    ExceptionPointer() {
        ContextRecord = new CONTEXT();
        ExceptionRecord = new EXCEPTION_RECORD();
    }

    ~ExceptionPointer() {
        delete ContextRecord;
        delete ExceptionRecord;
    }
};

void createMinidump(_EXCEPTION_POINTERS* exception_pointers) {
    auto file_name = String::toStdWString(String::format("{}-crashdump.dmp", getSequentialUuid()));

    auto file_ = CreateFileW(file_name.c_str(),
        GENERIC_WRITE,
        0, 
        nullptr,
        CREATE_ALWAYS, 
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if ((file_ == nullptr) || (file_ == INVALID_HANDLE_VALUE)) {
        return;
    }

    FileHandle crashdump(file_);
    MINIDUMP_EXCEPTION_INFORMATION mdei{};
    mdei.ThreadId = ::GetCurrentThreadId();
    mdei.ExceptionPointers = exception_pointers;
    mdei.ClientPointers = FALSE;

    MINIDUMP_TYPE mdt = MiniDumpNormal;
    ::MiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        crashdump.get(),
        mdt,
        (exception_pointers ? &mdei : nullptr),
        nullptr,
        nullptr
    );
}
#endif

class CrashHandler::CrashHandlerImpl {
public:
    CrashHandlerImpl() = default;
    ~CrashHandlerImpl() = default;

#ifdef XAMP_OS_WIN    
    static void dumpStackInfo(void* info) {
        std::lock_guard<std::recursive_mutex> guard{ mutex_ };

        StackTrace stack_trace;
        auto* exception_pointers = static_cast<PEXCEPTION_POINTERS>(info);        

        const auto code = exception_pointers->ExceptionRecord->ExceptionCode;
        if (code == EXCEPTION_STACK_OVERFLOW) {
            return;
        }

        const auto itr = kIgnoreExceptionCode.find(exception_pointers->ExceptionRecord->ExceptionCode);
        if (itr != kIgnoreExceptionCode.end()) {
            XAMP_LOG_TRACE("Ignore exception code: {}({:#010X}) {}",
                itr->second, itr->first, stack_trace.captureStack());
            return;
        }

        const auto itr2 = kWellKnownExceptionCode.find(code);
        if (itr2 != kWellKnownExceptionCode.end()) {
            XAMP_LOG_DEBUG("Uncaught exception: {} {}\r\n",
                (*itr2).second, stack_trace.captureStack());
        }
        else {
            XAMP_LOG_DEBUG("Uncaught exception: {:#010X} ({}) {}\r\n",
                code, GetPlatformErrorMessage(code), stack_trace.captureStack());
        }

        createMinidump(exception_pointers);
    }

    static void dumpCurrentExceptionStack() {
        ExceptionPointer exception_pointers;
        getExceptionPointers(0, &exception_pointers);
        dumpStackInfo(&exception_pointers);
    }

    static LONG vectoredHandler(PEXCEPTION_POINTERS exception_pointers) {
        dumpStackInfo(exception_pointers);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    static void terminateHandler() {
        dumpCurrentExceptionStack();
    }

    static void invalidParameterHandler(const wchar_t* expression,
        const wchar_t* function, const wchar_t* file_,
        unsigned int line, uintptr_t reserved) {
        dumpCurrentExceptionStack();
    }

    // CRT SIGABRT signal handler
    static void sigabrtHandler(int32_t) {
        // Caught SIGABRT C++ signal
        dumpCurrentExceptionStack();
    }

    // CRT sigint signal handler
    static void sigintHandler(int32_t) {
        // Interruption (SIGINT)
        dumpCurrentExceptionStack();
    }

    static void sigillHandler(int32_t) {
        dumpCurrentExceptionStack();
    }

    // CRT SIGTERM signal handler
    static void sigtermHandler(int32_t) {
        // Termination request (SIGTERM)
        dumpCurrentExceptionStack();
    }

    // CRT SIGFPE signal handler
    static void sigfpeHandler(int32_t) {
        // Floating point exception (SIGFPE)
        auto* exception_pointers = static_cast<PEXCEPTION_POINTERS>(_pxcptinfoptrs);
        dumpStackInfo(exception_pointers);
    }

    // CRT SIGSEGV signal handler
    static void sigsegvHandler(int32_t) {
        auto* exception_pointers = static_cast<PEXCEPTION_POINTERS>(_pxcptinfoptrs);
        dumpStackInfo(exception_pointers);
    }

    static int newHandler(size_t) {
        dumpCurrentExceptionStack();
        return 0;
    }

    static void getExceptionPointers(const DWORD exception_code, const ExceptionPointer* exception_pointers) {
        CONTEXT context_record{};
        ::RtlCaptureContext(&context_record);

        MemoryCopy(exception_pointers->ContextRecord, &context_record, sizeof(CONTEXT));
        MemorySet(exception_pointers->ExceptionRecord, 0, sizeof(EXCEPTION_RECORD));

        exception_pointers->ExceptionRecord->ExceptionCode = exception_code;
        exception_pointers->ExceptionRecord->ExceptionAddress = ::_ReturnAddress();
    }

    void setProcessExceptionHandlers() {
        //XAMP_LOG_DEBUG("Install process exception handler.");

        // Vectored Exception Handling (VEH) is an extension to structured exception handling.
        ::AddVectoredExceptionHandler(0, vectoredHandler);

        // Catch new operator memory allocation exceptions
        ::_set_new_handler(newHandler);

        // Catch invalid parameter exceptions.
        ::_set_invalid_parameter_handler(invalidParameterHandler);

        // Set up C++ signal handlers
        _set_abort_behavior(0, _CALL_REPORTFAULT | _WRITE_ABORT_MSG);

        // Catch an abnormal program termination
        (void)::signal(SIGABRT, sigabrtHandler);

        // Catch illegal instruction handler
        (void)::signal(SIGILL, sigillHandler);

        (void)::signal(SIGSEGV, sigsegvHandler);
    }

    void setThreadExceptionHandlers() {
        //XAMP_LOG_INFO("Install thread exception handler.");

        // C++ terminate handler 是「每個 thread 各自一份」，
        // 所以新 thread 建立後，要呼叫一次這個函式。
        ::set_terminate(terminateHandler);
    }

#else
    static void dumpStackInfo(void* info) {
    }

    void setProcessExceptionHandlers() {
        installSignalHandler();
    }

    void setThreadExceptionHandlers() {
        installSignalHandler();
    }

    static bool haveSiginfo(int signum) {
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
            XAMP_LOG_DEBUG("Restore failed in test for SA_SIGINFO: {}", strerror(errno));
        }

        return result;
    }

    static void logCrashSignal(int signum, const siginfo_t* info) {
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

        XAMP_LOG_ERROR("Fatal signal {} ({}) code:{} address:{}",
            signum,
            signal_name,
            info ? info->si_code : 0,
            info ? info->si_addr : nullptr);
        StackTrace trace;
        XAMP_LOG_ERROR("{}", trace.captureStack());
    }

    static void crashSignalHandler(int signal_number, siginfo_t* info, void*) {
        if (!haveSiginfo(signal_number)) {
            info = nullptr;
        }

        logCrashSignal(signal_number, info);

        // reset signal to SIG_DFL
        ::signal(signal_number, SIG_DFL);

        std::exit(0);
    }

    void installSignalHandler() {
        struct sigaction action;
        MemorySet(&action, 0, sizeof(action));

        sigemptyset(&action.sa_mask);
        action.sa_sigaction = crashSignalHandler;
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
    static std::recursive_mutex mutex_;
};

std::recursive_mutex CrashHandler::CrashHandlerImpl::mutex_;

CrashHandler::CrashHandler()
	: impl_(makeAlign<CrashHandlerImpl>()) {
}

XAMP_PIMPL_IMPL(CrashHandler)

void CrashHandler::setProcessExceptionHandlers() {
    if (!impl_) {
        return;
    }
    impl_->setProcessExceptionHandlers();
}

void CrashHandler::setThreadExceptionHandlers() {
    if (!impl_) {
        return;
    }
    impl_->setThreadExceptionHandlers();
}

void CrashHandler::dumpStackInfo(void* info) {
    CrashHandlerImpl::dumpStackInfo(info);
}

void CrashHandler::cleanup() {
	impl_.reset();
}

XAMP_BASE_NAMESPACE_END
