#include <thread>

#include <base/dll.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/platform.h>

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
#endif

void SetThreadName(std::string const& name) noexcept {
#ifdef XAMP_OS_WIN
    WinHandle thread(::GetCurrentThread());

    try {
        // At Windows 10 1607 Supported.
        // The SetThreadDescription API works even if no debugger is attached.
        DllFunction<HRESULT(HANDLE, PCWSTR)>
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

void SetThreadAffinity(std::thread& thread, int32_t core) noexcept {
#ifdef XAMP_OS_WIN
#if (_WIN32_WINNT >= 0x0601)
    const auto groups = ::GetActiveProcessorGroupCount();
    auto total_processors = 0, group = 0, number = 0;
    for (int i = 0; i < groups; i++) {
	    const auto processors = ::GetActiveProcessorCount(i);
        if (total_processors + processors > core) {
            group = i;
            number = core - total_processors;
            break;
        }
        total_processors += processors;
	}

    GROUP_AFFINITY group_affinity;
    group_affinity.Group = static_cast<WORD>(group);
    group_affinity.Mask = static_cast<uint64_t>(1) << number;
    group_affinity.Reserved[0] = 0;
    group_affinity.Reserved[1] = 0;
    group_affinity.Reserved[2] = 0;
    if (!::SetThreadGroupAffinity(thread.native_handle(), &group_affinity, nullptr)) {
        XAMP_LOG_DEBUG("cannot set thread group affinity");
    }

    PROCESSOR_NUMBER processor_number;
    processor_number.Group = group;
    processor_number.Number = number;
    processor_number.Reserved = 0;
    if (!::SetThreadIdealProcessorEx(thread.native_handle(), &processor_number, nullptr)) {
        XAMP_LOG_DEBUG("cannot set threadeal processor");
    }
#else
    auto mask = (static_cast<DWORD_PTR>(1) << core);
    ::SetThreadAffinityMask(thread.native_handle(), mask);
#endif
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

void SetCurrentThreadAffinity(int32_t core) noexcept {
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

std::string GetCurrentThreadId() {
    std::ostringstream ostr;
    ostr << std::this_thread::get_id();
    return ostr.str();
}

std::string MakeTempFileName() {
    char filename[64] = "xamp-podcast-cache-XXXXXX";
#if XAMP_OS_MAC
    mkstemp(filename);
#else
    _mktemp_s(filename, sizeof(filename));
#endif
    return filename;
}

#ifdef XAMP_OS_WIN
bool ExtendProcessWorkingSetSize(size_t size) noexcept {
    SIZE_T minimum = 0;
    SIZE_T maximum = 0;

    const WinHandle current_process(::GetCurrentProcess());

    if (!::GetProcessWorkingSetSize(current_process.get(), &minimum, &maximum)) {
        XAMP_LOG_DEBUG("GetProcessWorkingSetSize return failure! error:{}.", GetLastErrorMessage());
        return false;
    }   

    minimum += size;
    if (maximum < minimum + size) {
        maximum = minimum + size;
    }
    return ::SetProcessWorkingSetSize(current_process.get(), minimum, maximum);
}

static bool EnablePrivilege(std::string_view privilege, bool enable) noexcept {
    const WinHandle current_process(::GetCurrentProcess());

    WinHandle token;
    HANDLE process_token;

    if (!::OpenProcessToken(current_process.get(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        &process_token)) {       
        XAMP_LOG_DEBUG("OpenProcessToken return failure! error:{}.", GetLastErrorMessage());
        return false;
    }

    token.reset(process_token);
	
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    if (!::LookupPrivilegeValueA(nullptr,
        privilege.data(),
        &tp.Privileges[0].Luid)) {
        XAMP_LOG_DEBUG("LookupPrivilegeValueA return failure! error:{}.", GetLastErrorMessage());
        return false;
    }

    tp.Privileges->Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
    if (!::AdjustTokenPrivileges(token.get(),
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        nullptr,
        nullptr)) {
        XAMP_LOG_DEBUG("AdjustTokenPrivileges return failure! error:{}.", GetLastErrorMessage());
        return false;
    }

    return true;
}

bool InitWorkingSetSize(size_t working_set_size) noexcept {
    if (!EnablePrivilege("SeLockMemoryPrivilege", true)) {
        return false;
    }
    if (!ExtendProcessWorkingSetSize(working_set_size)) {
        XAMP_LOG_DEBUG("ExtendProcessWorkingSetSize return failure! error:{}.", GetLastErrorMessage());
        return false;
    }
    XAMP_LOG_DEBUG("InitWorkingSetSize {} success.", String::FormatBytes(working_set_size));
    return false;
}
#endif

}
