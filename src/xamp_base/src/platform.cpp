#include <thread>

#include <base/dll.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/rng.h>
#include <base/platform.h>
#include <base/assert.h>

#ifdef XAMP_OS_WIN
#pragma comment(lib, "rpcrt4.lib")
#include <rpcnterr.h>
#include <rpc.h>
#include <base/windows_handle.h>
#else
#include <uuid/uuid.h>
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
    for (DWORD i = 0; i < groups; i++) {
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
        XAMP_LOG_DEBUG("Can't set thread group affinity");
    }

    PROCESSOR_NUMBER processor_number;
    processor_number.Group = group;
    processor_number.Number = number;
    processor_number.Reserved = 0;
    if (!::SetThreadIdealProcessorEx(thread.native_handle(), &processor_number, nullptr)) {
        XAMP_LOG_DEBUG("Can't set threadeal processor");
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

void SetThreadPriority(ThreadPriority priority) noexcept {
#ifdef XAMP_OS_WIN
    auto thread_priority = THREAD_PRIORITY_NORMAL;
    switch (priority) {
    case ThreadPriority::BACKGROUND:
        // reduce CPU, page and IO priority for the current thread
        if (!::SetThreadPriority(::GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN)) {
            if (ERROR_THREAD_MODE_ALREADY_BACKGROUND == ::GetLastError()) {
                XAMP_LOG_DEBUG("Already in background mode");
                return;
            }
            XAMP_LOG_DEBUG("Failed to enter background mode! error: {}.", GetLastErrorMessage());
            return;
        }        
        if (!::SetThreadPriority(::GetCurrentThread(), THREAD_MODE_BACKGROUND_END)) {
            XAMP_LOG_DEBUG("Failed to enter background mode! error: {}.", GetLastErrorMessage());
        }
        break;
    case ThreadPriority::NORMAL:
        thread_priority = THREAD_PRIORITY_NORMAL;
        break;
    case ThreadPriority::HIGHEST:
        thread_priority = THREAD_PRIORITY_HIGHEST;
        break;
    }

    if (priority != ThreadPriority::BACKGROUND) {
        if (!::SetThreadPriority(::GetCurrentThread(), thread_priority)) {
            XAMP_LOG_DEBUG("Failed to enter background mode! error:{}.", GetLastErrorMessage());
        }
        return;
    }

    auto current_priority = ::GetThreadPriority(::GetCurrentThread());
    XAMP_LOG_DEBUG("Current thread priority is {}.", current_priority);
#else
#if !defined(PTHREAD_MIN_PRIORITY)
#define PTHREAD_MIN_PRIORITY  0
#endif
#if !defined(PTHREAD_MAX_PRIORITY)
#define PTHREAD_MAX_PRIORITY 31
#endif

    auto thread_priority = PTHREAD_MIN_PRIORITY;
    switch (priority) {
    case ThreadPriority::BACKGROUND:
        thread_priority = PTHREAD_MIN_PRIORITY;
        break;
    case ThreadPriority::NORMAL:
        thread_priority = PTHREAD_MAX_PRIORITY / 2;
        break;
    case ThreadPriority::HIGHEST:
        thread_priority = PTHREAD_MAX_PRIORITY;
        break;
    }
    struct sched_param thread_param;
    thread_param.sched_priority = thread_priority;
    ::pthread_setschedparam(::pthread_self(), SCHED_RR, &thread_param);
#endif
}

std::string MakeUuidString() {
#ifdef XAMP_OS_WIN
	UUID uuid;
    auto status = ::UuidCreate(&uuid);
    XAMP_ASSERT(status == RPC_S_OK);
    char* str = nullptr;
    status = ::UuidToStringA(&uuid, reinterpret_cast<RPC_CSTR*>(&str));
    XAMP_ASSERT(status == RPC_S_OK);
    std::string result = str;
    ::RpcStringFreeA(reinterpret_cast<RPC_CSTR*>(&str));
    return result;
#else
    uuid_t uuid;
    ::uuid_generate(uuid);
    char buf[37] = {0};
    ::uuid_unparse(uuid, buf);
    return buf;
#endif    
}

std::string GetCurrentThreadId() {
    std::ostringstream ostr;
    ostr << std::this_thread::get_id();
    return ostr.str();
}

std::string MakeTempFileName() {
    std::string filename = "xamp_temp_";
    static constexpr std::string_view kFilenameCharacters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (auto i = 0; i < 8; ++i) {
        filename += kFilenameCharacters[PRNG::GetInstance()(static_cast<size_t>(0), kFilenameCharacters.length() - 1)];
    }
    return filename;
}

bool IsDebuging() {
#ifdef XAMP_OS_WIN
    return ::IsDebuggerPresent();
#else
    return false;
#endif
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

bool SetProcessWorkingSetSize(size_t working_set_size) noexcept {
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

bool VirtualMemoryLock(void* address, size_t size) {
#ifdef XAMP_OS_WIN
    if (!::VirtualLock(address, size)) { // try lock memory!
        if (!ExtendProcessWorkingSetSize(size)) {
            throw PlatformSpecException("ExtendProcessWorkingSetSize return failure!");
        }
        if (!::VirtualLock(address, size)) {
            return false;
        }
    }
    return true;
#else
    return ::mlock(address, size) != -1;
#endif
}

bool VirtualMemoryUnLock(void* address, size_t size) {
#ifdef XAMP_OS_WIN
    return ::VirtualUnlock(address, size);
#else
    return ::munlock(address, size_) != -1;
#endif
}

}
