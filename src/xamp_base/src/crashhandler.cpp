#include <base/crashhandler.h>
#include <base/dll.h>
#include <base/memory.h>
#include <base/logger_impl.h>
#include <base/stacktrace.h>
#include <base/stl.h>
#include <base/fastmutex.h>
#include <base/logger.h>
#include <base/platfrom_handle.h>

#ifdef XAMP_OS_WIN
#include <new.h>
#include <dbghelp.h>
#include <minidumpapiset.h>
#include <eh_details.h>
#else
#include <signal.h>
#include <execinfo.h>
#endif

#include <mutex>

namespace xamp::base {

XAMP_DECLARE_LOG_NAME(CrashHandler);

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
#endif

class CrashHandler::CrashHandlerImpl {
public:
    CrashHandlerImpl() {
        logger_ = LoggerManager::GetInstance().GetLogger(kCrashHandlerLoggerName);
    }

#ifdef XAMP_OS_WIN
    static bool DumpMsvcException(const EXCEPTION_POINTERS * pointers) {
        using namespace stdx::detail;

        const auto& ehe_exception_record = *reinterpret_cast<EHExceptionRecord*>(pointers->ExceptionRecord);
        const auto* throw_information = ehe_exception_record.params.pThrowInfo;
        if (!throw_information) {
            XAMP_LOG_D(logger_, "No ThrowInfo exists. If this was a C++ exception, something must have corrupted it.");
            return false;
        }

        if (!ehe_exception_record.params.pExceptionObject) {
            return false;
        }

        const auto image_base = reinterpret_cast<uintptr_t>(ehe_exception_record.params.pThrowImageBase);
        const auto* catchable_type_array = reinterpret_cast<const CatchableTypeArray*>(
            static_cast<uintptr_t>(throw_information->pCatchableTypeArray) + image_base);
        const auto* catchable_type = reinterpret_cast<CatchableType*>(
            static_cast<uintptr_t>(catchable_type_array->arrayOfCatchableTypes[0]) + image_base);

        auto* exception_record = pointers->ExceptionRecord;
        HMODULE module = (exception_record->NumberParameters >= 4)
            ? reinterpret_cast<HMODULE>(exception_record->ExceptionInformation[3]) : nullptr;

#define RVA_TO_VA_(type, addr)  ( (type) ((uintptr_t) module + (uintptr_t) (addr)) )
        auto* type = RVA_TO_VA_(const std::type_info*, catchable_type->pType);
                
        auto *exception_object = reinterpret_cast<std::exception*>(
            adjustThis(catchable_type->thisDisplacement,
            ehe_exception_record.params.pExceptionObject));

        XAMP_LOG_D(logger_, "Catch MSVC exception class name:{} what:{}",
            type->name(),
            exception_object->what());

        return true;
    }

    static void Dump(void* info) {
        std::lock_guard<std::recursive_mutex> guard{ mutex_ };

        StackTrace stack_trace;
        const auto* exception_pointers = static_cast<PEXCEPTION_POINTERS>(info);

        auto itr = kIgnoreExceptionCode.find(exception_pointers->ExceptionRecord->ExceptionCode);
        if (itr != kIgnoreExceptionCode.end()) {
            /*if ((*itr).first == EXCEPTION_MSVC_CPP) {
                if (DumpMsvcException(exception_pointers)) {
                    return;
                }
            }*/
            XAMP_LOG_D(logger_, "Ignore exception code: {}({:#014X}){}",
                (*itr).second, (*itr).first, stack_trace.CaptureStack());
            return;
        }

        const auto code = exception_pointers->ExceptionRecord->ExceptionCode;
        if (code == EXCEPTION_STACK_OVERFLOW) {
            return;
        }

        auto itr2 = kWellKnownExceptionCode.find(code);
        if (itr2 != kWellKnownExceptionCode.end()) {
            XAMP_LOG_D(logger_, "Uncaught exception: {}{}\r\n", (*itr2).second, stack_trace.CaptureStack());
        }
        else {
            XAMP_LOG_D(logger_, "Uncaught exception: {:#014X}{}\r\n", code, stack_trace.CaptureStack());
        }
    }

    static void StackDump() {
        EXCEPTION_POINTERS exception_pointers{};
        GetExceptionPointers(0, &exception_pointers);

        if (exception_pointers.ContextRecord != nullptr && exception_pointers.ExceptionRecord != nullptr) {
            Dump(&exception_pointers);
        }        
    }

    static LONG SehHandler(PEXCEPTION_POINTERS exception_pointers) {
        Dump(exception_pointers);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    static LONG VectoredHandler(PEXCEPTION_POINTERS exception_pointers) {
        Dump(exception_pointers);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    static void TerminateHandler() {
        StackDump();
    }

    static void UnexpectedHandler() {
        StackDump();
    }

    static void PureCallHandler() {
        StackDump();
    }

    static void InvalidParameterHandler(const wchar_t* expression,
        const wchar_t* function, const wchar_t* file,
        unsigned int line, uintptr_t reserved) {
        StackDump();
    }

    static int NewHandler(size_t) {
        StackDump();
        return 0;
    }

    static void GetExceptionPointers(DWORD exception_code, EXCEPTION_POINTERS* exception_pointers) {
        CONTEXT context_record{};
        ::RtlCaptureContext(&context_record);

        MemoryCopy(exception_pointers->ContextRecord, &context_record, sizeof(CONTEXT));
        MemorySet(exception_pointers->ExceptionRecord, 0, sizeof(EXCEPTION_RECORD));

        exception_pointers->ExceptionRecord->ExceptionCode = exception_code;
        exception_pointers->ExceptionRecord->ExceptionAddress = _ReturnAddress();
    }

    void SetProcessExceptionHandlers() {
#ifdef XAMP_OS_WIN
        // Vectored Exception Handling (VEH) is an extension to structured exception handling.
        ::AddVectoredExceptionHandler(0, VectoredHandler);
#else
        InstallSignalHandler();
#endif
    }

    void SetThreadExceptionHandlers() {
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
            XAMP_LOG_D(logger_, "Restore failed in test for SA_SIGINFO: {}", strerror(errno));
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

        XAMP_LOG_E(logger_, "Fatal signal {} ({}){}{}",
            signum, signal_name, info->si_code, info->si_addr);
        StackTrace trace;
        XAMP_LOG_E(logger_, trace.CaptureStack());
    }

    static void CrashSignalHandler(int signal_number, siginfo_t* info, void*) {
        if (!HaveSiginfo(signal_number)) {
            info = nullptr;
        }

        LogCrashSignal(signal_number, info);

        // Reset signal to SIG_DFL
        ::signal(signal_number, SIG_DFL);

        std::exit(0);
    }

    void InstallSignalHandler() {
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
    static std::recursive_mutex mutex_;
    static LoggerPtr logger_;
};

std::recursive_mutex CrashHandler::CrashHandlerImpl::mutex_;
LoggerPtr CrashHandler::CrashHandlerImpl::logger_;

CrashHandler::CrashHandler()
	: impl_(MakeAlign<CrashHandlerImpl>()) {
}

XAMP_PIMPL_IMPL(CrashHandler)

void CrashHandler::SetProcessExceptionHandlers() {
#ifdef XAMP_OS_WIN
    impl_->SetProcessExceptionHandlers();
#else
    impl_->InstallSignalHandler();
#endif
}

void CrashHandler::SetThreadExceptionHandlers() {
#ifdef XAMP_OS_WIN
    impl_->SetThreadExceptionHandlers();
#endif
}

}
