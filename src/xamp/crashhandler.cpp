#if 0
#include <QDir>

#include <widget/str_utilts.h>
#include <base/exception.h>
#include <crashhandler.h>

#if defined(Q_OS_MAC)
#include <client/mac/handler/exception_handler.h>
#elif defined(Q_OS_WIN)
#include <client/windows/handler/exception_handler.h>
#endif

using xamp::base::PlatformSpecException;

class CrashHandler::CrashHandlerImpl {
public:
    CrashHandlerImpl() {

    }
    
    ~CrashHandlerImpl() {
        if (exceptionHandler != nullptr) {
            delete exceptionHandler;
        }
    }

    void init(const QString& dumpPath) {
#if defined(Q_OS_WIN32)
        exceptionHandler = new google_breakpad::ExceptionHandler(
            dumpPath.toStdWString(),
            nullptr,
            dumpCallback,
            nullptr,
            google_breakpad::ExceptionHandler::HANDLER_ALL
        );
#endif
    }

#ifdef Q_OS_WIN
    static bool launcher(std::wstring & program) {
        STARTUPINFO si = {};
        si.cb = sizeof si;
        PROCESS_INFORMATION pi = {};

        if (!::CreateProcess(nullptr, program.data(), 0, FALSE, 0,
            CREATE_DEFAULT_ERROR_MODE | CREATE_NO_WINDOW | DETACHED_PROCESS,
            0, 0, &si, &pi)) {
            throw PlatformSpecException();
        } else {
            if (::PostThreadMessage(pi.dwThreadId, WM_QUIT, 0, 0)) {
                ::CloseHandle(pi.hProcess);
                ::CloseHandle(pi.hThread);
            }
        }
    }

    static bool dumpCallback(const wchar_t* dump_dir,
        const wchar_t* minidump_id,
        void* context, 
        EXCEPTION_POINTERS* exinfo,
        MDRawAssertionInfo* assertion, 
        bool success) {

        QString minidumpFileName;
#if defined(Q_OS_WIN32)
        QDir minidumpDir = QDir(QString::fromWCharArray(reporter_.c_str()));
        minidumpFileName = minidumpDir.absoluteFilePath(QString::fromWCharArray(minidump_id) + Q_UTF8(".dmp"));
#elif defined(Q_OS_MAC)
        
#endif
        launcher(minidumpFileName.toStdWString());
        return success;
    }
#else
    bool launcher(const char* program, const char* path) {

    }
#endif

    static google_breakpad::ExceptionHandler* exceptionHandler;
    static std::wstring reporter_;
};

google_breakpad::ExceptionHandler* CrashHandler::CrashHandlerImpl::exceptionHandler = nullptr;
std::wstring CrashHandler::CrashHandlerImpl::reporter_;

CrashHandler& CrashHandler::instance() {
    static CrashHandler handler;
    return handler;
}
#endif
