#include <thread>

#include <base/dll.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/platform_thread.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#else
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#endif

namespace xamp::base {

#ifndef XAMP_OS_WIN
void SetThreadAffinity(pthread_t thread, int32_t core) {
    auto mach_thread = ::pthread_mach_thread_np(thread);
    thread_affinity_policy_data_t policy = { core };
    auto result = ::thread_policy_set(mach_thread,
                                      THREAD_AFFINITY_POLICY,
                                      reinterpret_cast<thread_policy_t>(&policy),
                                      1);
    if (result != KERN_SUCCESS) {
        XAMP_LOG_DEBUG("thread_policy_set return failure.");
    }
}
#else

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread Uuid (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadNameById(DWORD dwThreadID, char const* threadName) {
    static constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;

    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;

    __try {
        ::RaiseException(MS_VC_EXCEPTION,
                         0,
                         sizeof(info) / sizeof(ULONG_PTR),
                         reinterpret_cast<ULONG_PTR*>(&info));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

void SetRealtimeProcessPriority() {
}
#endif

void SetThreadName(std::string const& name) noexcept {
#ifdef XAMP_OS_WIN
    WinHandle thread(::GetCurrentThread());

    try {
        // At Windows 10 1607 Supported.
        // The SetThreadDescription API works even if no debugger is attached.
        DllFunction<HRESULT(HANDLE hThread, PCWSTR lpThreadDescription)>
            SetThreadDescription(LoadModule("Kernel32.dll"), "SetThreadDescription");

        if (SetThreadDescription) {
            SetThreadDescription(thread.get(), String::ToStdWString(name).c_str());
        }
    }
    catch (...) {
    }

    // The debugger needs to be around to catch the name in the exception.  If
    // there isn't a debugger, we are just needlessly throwing an exception.
    if (!::IsDebuggerPresent()) {
        return;
    }

    SetThreadNameById(::GetCurrentThreadId(), name.c_str());
#else
    // Mac OS X does not expose the length limit of the name, so
    // hardcode it.
    static constexpr int kMaxNameLength = 63;
    std::string shortened_name = name.substr(0, kMaxNameLength);
    ::pthread_setname_np(shortened_name.c_str());
#endif
}

void SetThreadAffinity(std::thread& thread, int32_t core) {
#ifdef XAMP_OS_WIN
    auto mask = (static_cast<DWORD_PTR>(1) << core);
    ::SetThreadAffinityMask(thread.native_handle(), mask);
#else
    thread_affinity_policy_data_t policy = { core };
    auto thread_id = thread.get_id();
    auto native_handle = *reinterpret_cast<std::thread::native_handle_type*>(&thread_id);
    ::thread_policy_set(
        ::pthread_mach_thread_np(native_handle),
        THREAD_AFFINITY_POLICY,
        reinterpret_cast<thread_policy_t>(&policy),
        1);
#endif
}

void SetCurrentThreadAffinity(int32_t core) {
#ifdef XAMP_OS_WIN
    WinHandle current_thread(::GetCurrentThread());
    auto mask = (static_cast<DWORD_PTR>(1) << core);
    ::SetThreadAffinityMask(current_thread.get(), mask);
#else
    thread_affinity_policy_data_t policy = { core };
    auto thread_id = std::this_thread::get_id();
    auto native_handle = *reinterpret_cast<std::thread::native_handle_type*>(&thread_id);
    ::thread_policy_set(
        ::pthread_mach_thread_np(native_handle),
        THREAD_AFFINITY_POLICY,
        reinterpret_cast<thread_policy_t>(&policy),
        1);
#endif
}

#ifdef XAMP_OS_WIN
bool ExtendProcessWorkingSetSize(size_t size) noexcept {
    SIZE_T minimum = 0;
    SIZE_T maximum = 0;

    const WinHandle current_process(::GetCurrentProcess());

    if (::GetProcessWorkingSetSize(current_process.get(), &minimum, &maximum)) {
        minimum += size;
        if (maximum < minimum + size) {
            maximum = minimum + size;
        }
        return ::SetProcessWorkingSetSize(current_process.get(), minimum, maximum);
    }
    return false;
}

static bool EnablePrivilege(std::string_view privilege, bool enable) noexcept {
    const WinHandle current_process(::GetCurrentProcess());

    WinHandle token;
    HANDLE process_token;

    if (::OpenProcessToken(current_process.get(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        &process_token)) {
        token.reset(process_token);

        TOKEN_PRIVILEGES tp;
        tp.PrivilegeCount = 1;
        if (!::LookupPrivilegeValueA(nullptr,
            privilege.data(),
            &tp.Privileges[0].Luid)) {
            XAMP_LOG_DEBUG("LookupPrivilegeValueA return failure! error:{}.", GetLastError());
            return false;
        }

        tp.Privileges->Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
        if (!::AdjustTokenPrivileges(token.get(),
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            nullptr,
            nullptr)) {
            XAMP_LOG_DEBUG("AdjustTokenPrivileges return failure! error:{}.", GetLastError());
            return false;
        }

        return true;
    }

    XAMP_LOG_DEBUG("OpenProcessToken return failure! error:{}.", GetLastError());
    return false;
}

void InitWorkingSetSize() {
    // https://social.msdn.microsoft.com/Forums/en-US/4890ecba-0325-4edf-99a8-bfc5d4f410e8/win10-major-issue-for-audio-processing-os-special-mode-for-small-buffer?forum=windowspro-audiodevelopment
    // Everything the SetProcessWorkingSetSize says is true. You should only lock what you need to lock.
    // And you need to lock everything you touch from the realtime thread. Because if the realtime thread
    // touches something that was paged out, you glitch.
    constexpr size_t kWorkingSetSize = 1024 * 1024 * 1024;
    if (EnablePrivilege("SeLockMemoryPrivilege", true)) {
        XAMP_LOG_DEBUG("EnableLockMemPrivilege success.");

        if (ExtendProcessWorkingSetSize(kWorkingSetSize)) {
            XAMP_LOG_DEBUG("ExtendProcessWorkingSetSize {} success.", String::FormatBytes(kWorkingSetSize));
        }
    }
}
#endif

}
