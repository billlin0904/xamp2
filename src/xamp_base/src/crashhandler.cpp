#include <mutex>

#include <base/crashhandler.h>
#include <base/dll.h>
#include <base/memory.h>
#include <base/logger_impl.h>
#include <base/stacktrace.h>
#include <base/stl.h>

#include <base/platfrom_handle.h>

#ifdef XAMP_OS_WIN
#include <new.h>
#include <dbghelp.h>
#include <minidumpapiset.h>
#else
#include <signal.h>
#include <execinfo.h>
#endif

namespace xamp::base {

CrashHandler::CrashHandler() = default;
CrashHandler::~CrashHandler() = default;

FastMutex CrashHandler::mutex_;

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

void CrashHandler::CreateMiniDump(EXCEPTION_POINTERS* exception_pointers) {
    FileHandle crashdump_file(::CreateFileW(
        L"crashdump.dmp",
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr));
    MINIDUMP_EXCEPTION_INFORMATION mei;
    MINIDUMP_CALLBACK_INFORMATION mci;

    // Write minidump to the file
    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = exception_pointers;
    mei.ClientPointers = FALSE;
    mci.CallbackRoutine = nullptr;
    mci.CallbackParam = nullptr;

    WinHandle process(::GetCurrentProcess());

    ::MiniDumpWriteDump(
        process.get(),
        ::GetCurrentProcessId(),
        crashdump_file.get(),
        MiniDumpNormal,
        &mei,
        nullptr,
        &mci);
}

void CrashHandler::Dump(void* info) {
    std::lock_guard<FastMutex> guard{ mutex_ };

    const auto* exception_pointers = static_cast<PEXCEPTION_POINTERS>(info);

    auto itr = kIgnoreExceptionCode.find(exception_pointers->ExceptionRecord->ExceptionCode);
    if (itr != kIgnoreExceptionCode.end()) {
        XAMP_LOG_DEBUG("Ignore exception code: {}({:#014X})", (*itr).second, (*itr).first);
        return;
    }
    
    //CreateMiniDump(exception_pointers);

    const auto code = exception_pointers->ExceptionRecord->ExceptionCode;
    if (code == EXCEPTION_STACK_OVERFLOW) {
        return;
    }

    auto itr2 = kWellKnownExceptionCode.find(code);
    if (itr2 != kWellKnownExceptionCode.end()) {
        XAMP_LOG_DEBUG("Uncaught exception: {}{}\r\n", (*itr2).second, StackTrace{}.CaptureStack());
    }
    else {
        XAMP_LOG_DEBUG("Uncaught exception: {:#014X}{}\r\n", code, StackTrace{}.CaptureStack());
    }

    //Logger::GetInstance().Shutdown();
    //WinHandle process(::GetCurrentProcess());
    //::TerminateProcess(process.get(), 1);
}

void CrashHandler::StackDump() {
    EXCEPTION_POINTERS exception_pointers{};
    GetExceptionPointers(0, &exception_pointers);
    Dump(&exception_pointers);
}

LONG CrashHandler::SehHandler(PEXCEPTION_POINTERS exception_pointers) {
    Dump(exception_pointers);
    return EXCEPTION_EXECUTE_HANDLER;
}

LONG CrashHandler::VectoredHandler(PEXCEPTION_POINTERS exception_pointers) {
    Dump(exception_pointers);
    return EXCEPTION_EXECUTE_HANDLER;
}

void CrashHandler::TerminateHandler() {
    StackDump();
}

void CrashHandler::UnexpectedHandler() {
    StackDump();
}

void CrashHandler::PureCallHandler() {
    StackDump();
}

void CrashHandler::InvalidParameterHandler(const wchar_t* expression,
                                           const wchar_t* function, const wchar_t* file,
                                           unsigned int line, uintptr_t reserved) {
    StackDump();
}

int CrashHandler::NewHandler(size_t) {
    StackDump();
    return 0;
}

void CrashHandler::GetExceptionPointers(DWORD exception_code, EXCEPTION_POINTERS* exception_pointers) {
    CONTEXT context_record{};
	::RtlCaptureContext(&context_record);

	MemoryCopy(exception_pointers->ContextRecord, &context_record, sizeof(CONTEXT));
    MemorySet(exception_pointers->ExceptionRecord, 0, sizeof(EXCEPTION_RECORD));

    exception_pointers->ExceptionRecord->ExceptionCode = exception_code;
    exception_pointers->ExceptionRecord->ExceptionAddress = _ReturnAddress();
}
#endif

void CrashHandler::SetProcessExceptionHandlers() {
#ifdef XAMP_OS_WIN
    // Vectored Exception Handling (VEH) is an extension to structured exception handling.
    ::AddVectoredExceptionHandler(0, VectoredHandler);
#if 0
    // Vectored Exception Handling (VEH) is an extension to structured exception handling.
    ::AddVectoredExceptionHandler(0, VectoredHandler);

    // Install top-level SEH handler
	::SetUnhandledExceptionFilter(SehHandler);

    // Catch pure virtual function calls.
    // Because there is one _purecall_handler for the whole process, 
    // calling this function immediately impacts all threads. The last 
    // caller on any thread sets the handler. 
    // http://msdn.microsoft.com/en-us/library/t296ys27.aspx
	::_set_purecall_handler(PureCallHandler);

    // Catch new operator memory allocation exceptions
    ::_set_new_handler(NewHandler);

    // Catch invalid parameter exceptions.
    ::_set_invalid_parameter_handler(InvalidParameterHandler);

    // Set up C++ signal handlers
    ::_set_abort_behavior(_CALL_REPORTFAULT, _CALL_REPORTFAULT);
#endif
#else
    InstallSignalHandler();
#endif
}

void CrashHandler::SetThreadExceptionHandlers() {
#ifdef XAMP_OS_WIN

    // Catch terminate() calls. 
    // In a multithreaded environment, terminate functions are maintained 
    // separately for each thread. Each new thread needs to install its own 
    // terminate function. Thus, each thread is in charge of its own termination handling.
    // http://msdn.microsoft.com/en-us/library/t6fk7h29.aspx
    ::set_terminate(TerminateHandler);
#if 1
    // Catch unexpected() calls.
    // In a multithreaded environment, unexpected functions are maintained 
    // separately for each thread. Each new thread needs to install its own 
    // unexpected function. Thus, each thread is in charge of its own unexpected handling.
    // http://msdn.microsoft.com/en-us/library/h46t5b69.aspx  
    ::set_unexpected(UnexpectedHandler);
#endif
#else
    InstallSignalHandler();
#endif
}

#ifndef XAMP_OS_WIN
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

void CrashHandler::CrashSignalHandler(int signal_number, siginfo_t* info, void*) {
    if (!HaveSiginfo(signal_number)) {
        info = nullptr;
    }

    LogCrashSignal(signal_number, info);

    // Reset signal to SIG_DFL
    ::signal(signal_number, SIG_DFL);

    std::exit(0);
}

void CrashHandler::InstallSignalHandler() {
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

}
