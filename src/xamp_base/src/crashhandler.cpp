#include <base/crashhandler.h>
#include <base/dll.h>
#include <base/fs.h>
#include <base/memory.h>
#include <base/logger.h>
#include <base/stacktrace.h>
#include <base/stl.h>
#include <base/fastmutex.h>
#include <base/platfrom_handle.h>

#ifdef XAMP_OS_WIN
#include <new.h>
#include <dbghelp.h>
#include <cstdlib>
#else
#include <signal.h>
#include <execinfo.h>
#endif

#include <atomic>
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

XAMP_MAKE_ENUM(CrashSource,
    kSeh,
    kVectored,
    kTerminate,
    kInvalidParameter,
    kNewHandler,
    kSignalAbort,
    kSignalFloatingPoint,
    kSignalIllegalInstruction,
    kSignalSegv,
    kSignalTerm)

struct CrashReportInfo {
    CrashSource source;
    DWORD code{ 0 };
    PEXCEPTION_POINTERS exception_pointers{ nullptr };
    const wchar_t* expression{ nullptr };
    const wchar_t* function{ nullptr };
    const wchar_t* file{ nullptr };
    unsigned int line{ 0 };
};

Path makeCrashDumpPath() {
    const Path crash_dump_dir = Path("logs") / Path("crashdump");
    std::error_code ec;
    Fs::create_directories(crash_dump_dir, ec);
    return crash_dump_dir / String::toStdWString(String::format("{}-crashdump.dmp", getSequentialUuid()));
}

void createMinidump(_EXCEPTION_POINTERS* exception_pointers) {
    auto file_name = makeCrashDumpPath().wstring();

    auto file_ = ::CreateFileW(file_name.c_str(),
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
    static bool isIgnoredException(DWORD code) {
        return kIgnoreExceptionCode.find(code) != kIgnoreExceptionCode.end();
    }

    static DWORD getCrashCode(const CrashReportInfo& info) {
        if (info.exception_pointers != nullptr && info.exception_pointers->ExceptionRecord != nullptr) {
            return info.exception_pointers->ExceptionRecord->ExceptionCode;
        }
        return info.code;
    }

    static void logCrashReport(const CrashReportInfo& info) {
        StackTrace stack_trace;
        const auto code = getCrashCode(info);

        const auto itr = kIgnoreExceptionCode.find(code);
        if (itr != kIgnoreExceptionCode.end()) {
            XAMP_LOG_TRACE("Ignore exception source:{} code:{}({:#010X}) {}",
                enumToString(info.source),
                itr->second,
                itr->first,
                stack_trace.captureStack());
            return;
        }

        const auto itr2 = kWellKnownExceptionCode.find(code);
        if (itr2 != kWellKnownExceptionCode.end()) {
            XAMP_LOG_DEBUG("Uncaught exception source:{} code:{} {}\r\n",
                enumToString(info.source),
                (*itr2).second,
                stack_trace.captureStack());
        }
        else {
            XAMP_LOG_DEBUG("Uncaught exception source:{} code:{:#010X} ({}) {}\r\n",
                enumToString(info.source),
                code,
                getPlatformErrorMessage(code),
                stack_trace.captureStack());
        }

        if (info.expression != nullptr || info.function != nullptr || info.file != nullptr) {
            XAMP_LOG_DEBUG("Invalid parameter expression:{} function:{} file:{} line:{}",
                info.expression != nullptr ? String::toUtf8String(info.expression) : std::string{},
                info.function != nullptr ? String::toUtf8String(info.function) : std::string{},
                info.file != nullptr ? String::toUtf8String(info.file) : std::string{},
                info.line);
        }
    }

    static void writeCrashReport(const CrashReportInfo& info, bool write_minidump) {
        const auto code = getCrashCode(info);
        if (isIgnoredException(code)) {
            logCrashReport(info);
            return;
        }

        static std::atomic_flag report_in_progress = ATOMIC_FLAG_INIT;
        if (write_minidump && report_in_progress.test_and_set(std::memory_order_acq_rel)) {
            return;
        }

        std::lock_guard<std::recursive_mutex> guard{ mutex_ };
        logCrashReport(info);

        if (write_minidump) {
            createMinidump(info.exception_pointers);
        }
    }

    static void dumpStackInfo(void* info) {
        auto* exception_pointers = static_cast<PEXCEPTION_POINTERS>(info);
        const auto code = exception_pointers && exception_pointers->ExceptionRecord
            ? exception_pointers->ExceptionRecord->ExceptionCode
            : 0;
        CrashReportInfo report_info{
            .source = CrashSource::kSeh,
            .code = code,
            .exception_pointers = exception_pointers,
        };
        writeCrashReport(report_info, true);
    }

    static void getExceptionPointers(const DWORD exception_code, ExceptionPointer* exception_pointers) {
        CONTEXT context_record{};
        ::RtlCaptureContext(&context_record);

        MemoryCopy(exception_pointers->ContextRecord, &context_record, sizeof(CONTEXT));
        MemorySet(exception_pointers->ExceptionRecord, 0, sizeof(EXCEPTION_RECORD));

        exception_pointers->ExceptionRecord->ExceptionCode = exception_code;
        exception_pointers->ExceptionRecord->ExceptionAddress = ::_ReturnAddress();
    }

    static void dumpCurrentExceptionStack(CrashSource source, DWORD exception_code = 0) {
        ExceptionPointer exception_pointers;
        getExceptionPointers(exception_code, &exception_pointers);
        CrashReportInfo info{
            .source = source,
            .code = exception_code,
            .exception_pointers = &exception_pointers,
        };
        writeCrashReport(info, true);
    }

    static DWORD WINAPI stackOverflowDumpThread(void* parameter) {
        auto* exception_pointers = static_cast<PEXCEPTION_POINTERS>(parameter);
        CrashReportInfo info{
            .source = CrashSource::kSeh,
            .code = EXCEPTION_STACK_OVERFLOW,
            .exception_pointers = exception_pointers,
        };
        writeCrashReport(info, true);
        return 0;
    }

    static void writeStackOverflowCrashReportOnNewThread(PEXCEPTION_POINTERS exception_pointers) {
        auto thread = ::CreateThread(nullptr,
            0,
            stackOverflowDumpThread,
            exception_pointers,
            0,
            nullptr);
        if (thread == nullptr) {
            CrashReportInfo info{
                .source = CrashSource::kSeh,
                .code = EXCEPTION_STACK_OVERFLOW,
                .exception_pointers = exception_pointers,
            };
            writeCrashReport(info, true);
            return;
        }

        ::WaitForSingleObject(thread, INFINITE);
        ::CloseHandle(thread);
    }

    static LONG WINAPI sehHandler(PEXCEPTION_POINTERS exception_pointers) {
        const auto code = exception_pointers && exception_pointers->ExceptionRecord
            ? exception_pointers->ExceptionRecord->ExceptionCode
            : 0;

        if (code == EXCEPTION_STACK_OVERFLOW) {
            writeStackOverflowCrashReportOnNewThread(exception_pointers);
            return EXCEPTION_EXECUTE_HANDLER;
        }

        CrashReportInfo info{
            .source = CrashSource::kSeh,
            .code = code,
            .exception_pointers = exception_pointers,
        };
        writeCrashReport(info, true);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    static LONG WINAPI vectoredHandler(PEXCEPTION_POINTERS exception_pointers) {
        CrashReportInfo info{
            .source = CrashSource::kVectored,
            .exception_pointers = exception_pointers,
        };
        writeCrashReport(info, false);
        return EXCEPTION_CONTINUE_SEARCH;
    }

    static void terminateHandler() {
        dumpCurrentExceptionStack(CrashSource::kTerminate);
    }

    static void invalidParameterHandler(const wchar_t* expression,
        const wchar_t* function, const wchar_t* file_,
        unsigned int line, uintptr_t reserved) {
        (void)reserved;
        ExceptionPointer exception_pointers;
        getExceptionPointers(0, &exception_pointers);
        CrashReportInfo info{
            .source = CrashSource::kInvalidParameter,
            .exception_pointers = &exception_pointers,
            .expression = expression,
            .function = function,
            .file = file_,
            .line = line,
        };
        writeCrashReport(info, true);
    }

    static void pureCallHandler() {
        dumpCurrentExceptionStack(CrashSource::kTerminate);
    }

    // CRT SIGABRT signal handler
    static void sigabrtHandler(int32_t) {
        dumpCurrentExceptionStack(CrashSource::kSignalAbort);
    }

    static void sigillHandler(int32_t) {
        dumpCurrentExceptionStack(CrashSource::kSignalIllegalInstruction);
    }

    // CRT SIGTERM signal handler
    static void sigtermHandler(int32_t) {
        dumpCurrentExceptionStack(CrashSource::kSignalTerm);
    }

    // CRT SIGFPE signal handler
    static void sigfpeHandler(int32_t) {
        auto* exception_pointers = static_cast<PEXCEPTION_POINTERS>(_pxcptinfoptrs);
        CrashReportInfo info{
            .source = CrashSource::kSignalFloatingPoint,
            .exception_pointers = exception_pointers,
        };
        writeCrashReport(info, true);
    }

    // CRT SIGSEGV signal handler
    static void sigsegvHandler(int32_t) {
        auto* exception_pointers = static_cast<PEXCEPTION_POINTERS>(_pxcptinfoptrs);
        CrashReportInfo info{
            .source = CrashSource::kSignalSegv,
            .exception_pointers = exception_pointers,
        };
        writeCrashReport(info, true);
    }

    static int newHandler(size_t) {
        dumpCurrentExceptionStack(CrashSource::kNewHandler);
        return 0;
    }

    void setProcessExceptionHandlers() {
        // Vectored handler records first-chance context only; SEH/CRT handlers own the minidump path.
        ::AddVectoredExceptionHandler(0, vectoredHandler);
        ::SetUnhandledExceptionFilter(sehHandler);

        // Catch new operator memory allocation exceptions
        ::_set_new_handler(newHandler);

        // Catch pure virtual calls.
        ::_set_purecall_handler(pureCallHandler);

        // Catch invalid parameter exceptions.
        ::_set_invalid_parameter_handler(invalidParameterHandler);

        // Set up C++ signal handlers
        _set_abort_behavior(0, _CALL_REPORTFAULT | _WRITE_ABORT_MSG);

        // Catch an abnormal program termination
        (void)::signal(SIGABRT, sigabrtHandler);

        // Catch illegal instruction handler
        (void)::signal(SIGILL, sigillHandler);

        (void)::signal(SIGFPE, sigfpeHandler);
        (void)::signal(SIGSEGV, sigsegvHandler);
        (void)::signal(SIGTERM, sigtermHandler);
    }

    void setThreadExceptionHandlers() {
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
