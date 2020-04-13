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
void SetRealtimeProcessPriority() {
    auto mach_thread_id = mach_thread_self();

    // Make thread fixed priority.
    thread_extended_policy_data_t policy;
    policy.timeshare = 0;  // Set to 1 for a non-fixed thread.
    auto result =
        thread_policy_set(mach_thread_id,
                          THREAD_EXTENDED_POLICY,
                          reinterpret_cast<thread_policy_t>(&policy),
                          THREAD_EXTENDED_POLICY_COUNT);
    if (result != KERN_SUCCESS) {
        XAMP_LOG_DEBUG("Make thread fixed priority!");
        return;
    }

    // Set to relatively high priority.
    thread_precedence_policy_data_t precedence;
    precedence.importance = 63;
    result = thread_policy_set(mach_thread_id,
                               THREAD_PRECEDENCE_POLICY,
                               reinterpret_cast<thread_policy_t>(&precedence),
                               THREAD_PRECEDENCE_POLICY_COUNT);
    if (result != KERN_SUCCESS) {
        XAMP_LOG_DEBUG("Set to relatively high priority failure!");
        return;
    }

    // Define the guaranteed and max fraction of time for the audio thread.
    // These "duty cycle" values can range from 0 to 1.  A value of 0.5
    // means the scheduler would give half the time to the thread.
    // These values have empirically been found to yield good behavior.
    // Good means that audio performance is high and other threads won't starve.
    constexpr double kGuaranteedAudioDutyCycle = 0.75;
    constexpr double kMaxAudioDutyCycle = 0.85;

    // About 128 frames @44.1KHz
    constexpr double kTimeQuantum = 2.9;

    // Time guaranteed each quantum.
    constexpr double kAudioTimeNeeded = kGuaranteedAudioDutyCycle * kTimeQuantum;

    // Maximum time each quantum.
    constexpr double kMaxTimeAllowed = kMaxAudioDutyCycle * kTimeQuantum;

    // Get the conversion factor from milliseconds to absolute time
    // which is what the time-constraints call needs.
    mach_timebase_info_data_t tb_info;
    mach_timebase_info(&tb_info);
    auto ms_to_abs_time =
        (static_cast<double>(tb_info.denom) / tb_info.numer) * 1000000;

    thread_time_constraint_policy_data_t time_constraints;
    time_constraints.period = kTimeQuantum * ms_to_abs_time;
    time_constraints.computation = kAudioTimeNeeded * ms_to_abs_time;
    time_constraints.constraint = kMaxTimeAllowed * ms_to_abs_time;
    time_constraints.preemptible = 0;

    result = thread_policy_set(mach_thread_id,
                               THREAD_TIME_CONSTRAINT_POLICY,
                               reinterpret_cast<thread_policy_t>(&time_constraints),
                               THREAD_TIME_CONSTRAINT_POLICY_COUNT);
    if (result != KERN_SUCCESS) {
        XAMP_LOG_DEBUG("thread_policy_set return failure!");
        return;
    }
}
#else

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadNameById(DWORD dwThreadID, const char* threadName) {
    constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;

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

void SetThreadName(const std::string& name) noexcept {
#ifdef XAMP_OS_WIN
    WinHandle thread(::GetCurrentThread());

    try {
        // At Windows 10 1607 Supported.
        // The SetThreadDescription API works even if no debugger is attached.
        DllFunction<HRESULT(HANDLE hThread, PCWSTR lpThreadDescription)>
            SetThreadDescription(LoadDll("Kernel32.dll"), "SetThreadDescription");

        if (SetThreadDescription) {
            SetThreadDescription(thread.get(), ToStdWString(name).c_str());
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
    const int kMaxNameLength = 63;
    std::string shortened_name = name.substr(0, kMaxNameLength);
    pthread_setname_np(shortened_name.c_str());
#endif
}

void SetCurrentThreadName(size_t index) {
    std::ostringstream ostr;
    ostr << "Streaming Thread(" << index << ")";
    SetThreadName(ostr.str());
}

void SetThreadAffinity(std::thread& thread, int32_t core) {
#ifdef XAMP_OS_WIN
    auto mask = (static_cast<DWORD_PTR>(1) << core);
    if (!::SetThreadAffinityMask(thread.native_handle(), mask)) {
        XAMP_LOG_DEBUG("SetThreadAffinityMask return failure!");
    }
#endif
}

}
