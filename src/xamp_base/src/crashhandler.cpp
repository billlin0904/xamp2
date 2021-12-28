#include <base/crashhandler.h>
#include <base/dll.h>
#include <base/memory.h>
#include <base/logger.h>
#include <base/stacktrace.h>

#ifdef XAMP_OS_WIN
#include <new.h>
#include <base/windows_handle.h>
#include <dbghelp.h>
#include <minidumpapiset.h>
#else
#include <signal.h>
#include <execinfo.h>
#endif

namespace xamp::base {

CrashHandler::CrashHandler() = default;
CrashHandler::~CrashHandler() = default;

#ifdef XAMP_OS_WIN
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

LONG CrashHandler::SehHandler(PEXCEPTION_POINTERS exception_pointers) {
    CreateMiniDump(exception_pointers);
    ::TerminateProcess(GetCurrentProcess(), 1);
    return EXCEPTION_EXECUTE_HANDLER;
}

LONG CrashHandler::VectoredHandler(PEXCEPTION_POINTERS exception_pointers) {
    CreateMiniDump(exception_pointers);
    ::TerminateProcess(GetCurrentProcess(), 1);
    return EXCEPTION_EXECUTE_HANDLER;
}

void CrashHandler::TerminateHandler() {
    EXCEPTION_POINTERS* exception_pointers = nullptr;
    GetExceptionPointers(0, &exception_pointers);
    CreateMiniDump(exception_pointers);
    WinHandle process(::GetCurrentProcess());
    ::TerminateProcess(process.get(), 1);
}

void CrashHandler::UnexpectedHandler() {
    EXCEPTION_POINTERS* exception_pointers = nullptr;
    GetExceptionPointers(0, &exception_pointers);
    CreateMiniDump(exception_pointers);
    WinHandle process(::GetCurrentProcess());
    ::TerminateProcess(process.get(), 1);
}

void CrashHandler::PureCallHandler() {
    EXCEPTION_POINTERS* exception_pointers = nullptr;
    GetExceptionPointers(0, &exception_pointers);
    CreateMiniDump(exception_pointers);
    WinHandle process(::GetCurrentProcess());
    ::TerminateProcess(process.get(), 1);
}

void CrashHandler::InvalidParameterHandler(const wchar_t* expression,
                                           const wchar_t* function, const wchar_t* file,
                                           unsigned int line, uintptr_t reserved) {
    EXCEPTION_POINTERS* exception_pointers = nullptr;
    GetExceptionPointers(0, &exception_pointers);
    CreateMiniDump(exception_pointers);
    WinHandle process(::GetCurrentProcess());
    ::TerminateProcess(process.get(), 1);
}

int CrashHandler::NewHandler(size_t) {
    EXCEPTION_POINTERS* exception_pointers = nullptr;
    GetExceptionPointers(0, &exception_pointers);
    CreateMiniDump(exception_pointers);
    WinHandle process(::GetCurrentProcess());
    ::TerminateProcess(process.get(), 1);
    return 0;
}

void CrashHandler::GetExceptionPointers(DWORD exception_code, EXCEPTION_POINTERS** exception_pointers) {
    EXCEPTION_RECORD temp{ 0 };
    CONTEXT stack_context_record{ 0 };

    ::RtlCaptureContext(&stack_context_record);
    temp.ExceptionCode = exception_code;
    temp.ExceptionAddress = ::_ReturnAddress();

    auto* exception_record = new EXCEPTION_RECORD();
    MemoryCopy(exception_record, &temp, sizeof(EXCEPTION_RECORD));
    auto* context_record = new CONTEXT();
    MemoryCopy(context_record, &stack_context_record, sizeof(CONTEXT));

    *exception_pointers = new EXCEPTION_POINTERS();
    (*exception_pointers)->ExceptionRecord = exception_record;
    (*exception_pointers)->ContextRecord = context_record;
}
#endif

void CrashHandler::SetProcessExceptionHandlers() {
#ifdef XAMP_OS_WIN
    // Vectored Exception Handling (VEH) is an extension to structured exception handling.
    ::AddVectoredExceptionHandler(0, VectoredHandler);

    // Install top-level SEH handler
	//::SetUnhandledExceptionFilter(SehHandler);

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

    // Catch unexpected() calls.
    // In a multithreaded environment, unexpected functions are maintained 
    // separately for each thread. Each new thread needs to install its own 
    // unexpected function. Thus, each thread is in charge of its own unexpected handling.
    // http://msdn.microsoft.com/en-us/library/h46t5b69.aspx  
    ::set_unexpected(UnexpectedHandler);
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
