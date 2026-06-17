#include <base/platform.h>

#include <base/dll.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/rng.h>
#include <base/assert.h>
#include <base/waitabletimer.h>
#include <base/exception.h>
#include <base/memory.h>
#include <base/platfrom_handle.h>
#include <base/shared_singleton.h>

#ifdef XAMP_OS_WIN
#include <rpcnterr.h>
#include <rpc.h>
#include <wincrypt.h>
#else
#include <pthread.h>
#include <uuid/uuid.h>
#include <sys/mman.h>
#endif

#ifdef XAMP_OS_MAC
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#endif

#ifdef XAMP_OS_LINUX
#include <cerrno>
#include <sched.h>
#include <cstring>
#endif

#include <bitset>
#include <thread>

#include <base/scopeguard.h>

#include <algorithm>
#include <limits>
#include <mutex>

#ifdef XAMP_OS_WIN
#include <tlhelp32.h>
#endif

#ifdef XAMP_OS_MAC
extern "C" int __ulock_wait(uint32_t operation, void* addr, uint64_t value,
                            uint32_t timeout); /* timeout is specified in microseconds */
extern "C" int __ulock_wake(uint32_t operation, void* addr, uint64_t wake_value);

#define UL_COMPARE_AND_WAIT	1
#define ULF_WAKE_ALL 0x00000100

template <typename t>
static int MacOSFutexWake(std::atomic<t>& to_wake, bool notify_one) {
    return ::__ulock_wake(UL_COMPARE_AND_WAIT | (notify_one ? 0 : ULF_WAKE_ALL), &to_wake, 0);
}
#endif

XAMP_BASE_NAMESPACE_BEGIN

namespace {
#ifdef XAMP_OS_WIN
    SIZE_T SaturatingAdd(SIZE_T lhs, size_t rhs) {
        const auto max_value = std::numeric_limits<SIZE_T>::max();
        if (rhs > max_value - lhs) {
            return max_value;
        }
        return lhs + rhs;
    }

    class WorkingSetLocker final {
    public:
        XAMP_DECLARE_SINGLETON_NAME()

        void recordLocked(size_t size) {
            if (size == 0) {
                return;
            }

            std::lock_guard lock{ mutex_ };
            active_locked_bytes_ = SaturatingAdd(active_locked_bytes_, size);
            XAMP_LOG_DEBUG("VirtualLock succeeded without working set resize. locked: {} active_locked: {}.",
                String::FormatBytes(size),
                String::FormatBytes(active_locked_bytes_));
        }

        bool reserveForLock(size_t size) {
            if (size == 0) {
                return true;
            }

            std::lock_guard lock{ mutex_ };
            if (!ensureInitialized()) {
                return false;
            }

            const auto requested_locked_bytes = SaturatingAdd(active_locked_bytes_, size);
            const auto target_minimum = SaturatingAdd(initial_minimum_, requested_locked_bytes);
            const auto target_maximum = std::max(initial_maximum_, target_minimum);

            if (requested_minimum_ < target_minimum || requested_maximum_ < target_maximum) {
                const auto current_process = ::GetCurrentProcess();
                if (!::SetProcessWorkingSetSize(current_process, target_minimum, target_maximum)) {
                    XAMP_LOG_DEBUG(
                        "SetProcessWorkingSetSize failed. locked: {} active_locked: {} target_minimum: {} target_maximum: {} error:{}.",
                        String::FormatBytes(size),
                        String::FormatBytes(active_locked_bytes_),
                        String::FormatBytes(target_minimum),
                        String::FormatBytes(target_maximum),
                        GetLastErrorMessage());
                    return false;
                }
                requested_minimum_ = target_minimum;
                requested_maximum_ = target_maximum;
                XAMP_LOG_DEBUG(
                    "SetProcessWorkingSetSize succeeded. locked: {} active_locked: {} minimum: {} maximum: {}.",
                    String::FormatBytes(size),
                    String::FormatBytes(requested_locked_bytes),
                    String::FormatBytes(requested_minimum_),
                    String::FormatBytes(requested_maximum_));
            }

            active_locked_bytes_ = requested_locked_bytes;
            return true;
        }

        void releaseLocked(size_t size) {
            if (size == 0) {
                return;
            }

            std::lock_guard lock{ mutex_ };
            active_locked_bytes_ = size >= active_locked_bytes_
                ? 0
                : active_locked_bytes_ - size;
            XAMP_LOG_DEBUG("VirtualUnlock released locked memory. unlocked: {} active_locked: {}.",
                String::FormatBytes(size),
                String::FormatBytes(active_locked_bytes_));
        }

    private:
        bool ensureInitialized() {
            if (initialized_) {
                return true;
            }

            const auto current_process = ::GetCurrentProcess();
            if (!::GetProcessWorkingSetSize(current_process, &initial_minimum_, &initial_maximum_)) {
                XAMP_LOG_DEBUG("GetProcessWorkingSetSize return failure! error:{}.", GetLastErrorMessage());
                return false;
            }

            requested_minimum_ = initial_minimum_;
            requested_maximum_ = initial_maximum_;
            initialized_ = true;
            XAMP_LOG_TRACE("initial process working set. minimum: {} maximum: {}.",
                String::FormatBytes(initial_minimum_),
                String::FormatBytes(initial_maximum_));
            return true;
        }

        std::mutex mutex_;
        bool initialized_{ false };
        SIZE_T initial_minimum_{ 0 };
        SIZE_T initial_maximum_{ 0 };
        SIZE_T requested_minimum_{ 0 };
        SIZE_T requested_maximum_{ 0 };
        SIZE_T active_locked_bytes_{ 0 };
    };

    void SetProcessPriority(const WinHandle& handle, ProcessPriority priority) {
        if (handle) {
            DWORD priority_class = NORMAL_PRIORITY_CLASS;
            if (priority == ProcessPriority::PRIORITY_BACKGROUND) {
                priority_class = IDLE_PRIORITY_CLASS;
            }
            else if (priority == ProcessPriority::PRIORITY_BACKGROUND_PERCEIVABLE) {
                priority_class = BELOW_NORMAL_PRIORITY_CLASS;
            }
            if (!::SetPriorityClass(handle.get(), priority_class)) {
                XAMP_LOG_DEBUG("Failed to set SetPriorityClass! error: {}.", GetLastErrorMessage());
                return;
            }
        }

        constexpr auto enable_eco_qos = true;
        PROCESS_POWER_THROTTLING_STATE power_throttling{};
        power_throttling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
        power_throttling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
        power_throttling.StateMask =
            (priority == ProcessPriority::PRIORITY_BACKGROUND) &&
            enable_eco_qos
            ? PROCESS_POWER_THROTTLING_EXECUTION_SPEED
            : 0;
        if (!::SetProcessInformation(handle.get(), ProcessPowerThrottling, &power_throttling, sizeof(power_throttling))) {
            XAMP_LOG_DEBUG("Failed to set SetProcessInformation! error: {}.", GetLastErrorMessage());
        }
    }
#endif

    uint64_t ToMilliseconds(const timespec* ts) {
        return static_cast<uint64_t>(ts->tv_sec) * 1000 + ts->tv_nsec / 1000000;
    }

    /*
     * Futex wait implementation.
     *
     * @param[out] to_wait_on The atomic variable to wait on.
     * @param[in] expected The expected value of the atomic variable.
     * @param[in] milliseconds The number of milliseconds to wait for.
     * @return true if the atomic variable was woken up, false if the wait timed out.
     */
    template <typename t>
    bool PlatformFutexWait(std::atomic<t>& to_wait_on, uint32_t& expected, uint32_t milliseconds) {
#ifdef XAMP_OS_WIN
        // 在 Windows 上，INFINITE 通常定義為 0xFFFFFFFF，表示無限等待
        return ::WaitOnAddress(&to_wait_on, &expected, sizeof(expected), milliseconds) != 0;
#elif defined(XAMP_OS_MAC)
        // 在 macOS 上，超時為 0 表示無限等待，時間單位為微秒
        uint32_t timeout_us = (milliseconds == kInfinity) ? 0 : milliseconds * 1000;
        return ::__ulock_wait(UL_COMPARE_AND_WAIT, &to_wait_on, expected, timeout_us) == 0;
#elif defined(XAMP_OS_LINUX)
        if (milliseconds == kInfinity) {
            to_wait_on.wait(expected, std::memory_order_acquire);
            return true;
        }

        const auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
        while (to_wait_on.load(std::memory_order_acquire) == expected) {
            if (std::chrono::steady_clock::now() >= timeout) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return true;
#endif
    }

    /*
    * Futex wake implementation.
    *
    * @param[out] to_wake The atomic variable to wake up.
    */
    template <typename t>
    void PlatformFutexWakeSingle(std::atomic<t>& to_wake) {
#ifdef XAMP_OS_WIN
        ::WakeByAddressSingle(&to_wake);
#elif defined (XAMP_OS_MAC)
        MacOSFutexWake(to_wake, true);
#elif defined(XAMP_OS_LINUX)
        to_wake.notify_one();
#endif
    }

    /*
    * Futex wake all implementation.
    *
    * @param[out] to_wake The atomic variable to wake up.
    */
    template <typename t>
	void PlatformFutexWakeAll(std::atomic<t>& to_wake) {
#ifdef XAMP_OS_WIN
        ::WakeByAddressAll(&to_wake);
#elif defined (XAMP_OS_MAC)
        MacOSFutexWake(to_wake, false);
#elif defined(XAMP_OS_LINUX)
        to_wake.notify_all();
#endif
    }
}

void AtomicWakeSingle(std::atomic<uint32_t>& to_wake) {
    PlatformFutexWakeSingle(to_wake);
}

void AtomicWakeAll(std::atomic<uint32_t>& to_wake) {
    PlatformFutexWakeAll(to_wake);
}

bool AtomicWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, uint32_t milliseconds) {
    return PlatformFutexWait(to_wait_on, expected, milliseconds);
}

int32_t AtomicWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, const timespec* to) {   
    if (to == nullptr) {
        if (!AtomicWait(to_wait_on, expected, kInfinity)) {
            errno = EINTR;
            return -1;
        }
        return 0;
    }

    // Check for invalid time-out values.
    XAMP_EXPECTS(to->tv_nsec >= 0);

    // Check for invalid time-out values.
    if (to->tv_nsec >= 1000000000) {
        errno = EINVAL;
        return -1;
    }

    // Check for time-outs that are too large to be represented in milliseconds.
    if (to->tv_sec >= 2147) {
        AtomicWait(to_wait_on, expected, 2147000000);
        return 0; /* time-out out of range, claim spurious wake-up */
    }

    // Wait for the specified time-out.
    if (!AtomicWait(to_wait_on, expected, static_cast<uint32_t>(ToMilliseconds(to)))) {
        errno = ETIMEDOUT;
        return -1;
    }
    return 0;
}

#if defined(XAMP_OS_MAC)
static void SetThreadAffinity(pthread_t thread, int32_t cpu_set) {
    auto mach_thread = ::pthread_mach_thread_np(thread);
    thread_affinity_policy_data_t policy = { cpu_set };
    auto result = ::thread_policy_set(mach_thread,
                                      THREAD_AFFINITY_POLICY,
                                      reinterpret_cast<thread_policy_t>(&policy),
                                      THREAD_AFFINITY_POLICY_COUNT);
    if (result != KERN_SUCCESS) {
        XAMP_LOG_DEBUG("thread_policy_set return failure ({}).", result);
    }
}
#elif defined(XAMP_OS_LINUX)
static void SetThreadAffinity(pthread_t thread, int32_t cpu_set) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_set, &cpuset);
    if (::pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0) {
        XAMP_LOG_DEBUG("pthread_setaffinity_np return failure.");
    }
}
#endif

void SetThreadName(std::wstring const& name) {
#ifdef XAMP_OS_WIN
	const WinHandle thread(::GetCurrentThread());
    ::SetThreadDescription(thread.get(), name.c_str());
#else
#ifdef XAMP_OS_MAC
    static constexpr int kMaxNameLength = 63;
#else
    static constexpr int kMaxNameLength = 15;
#endif
    const auto shortened_name = String::toUtf8String(name).substr(0, kMaxNameLength);
#ifdef XAMP_OS_MAC
    ::pthread_setname_np(shortened_name.c_str());
#else
    ::pthread_setname_np(::pthread_self(), shortened_name.c_str());
#endif
#endif
}

void SetThreadPriority(std::thread::native_handle_type handle, ThreadPriority priority) {
#ifdef XAMP_OS_WIN
    auto thread_priority = THREAD_PRIORITY_NORMAL;
    switch (priority) {
    case ThreadPriority::PRIORITY_BACKGROUND:
        if (::GetThreadPriority(handle) >= THREAD_PRIORITY_BELOW_NORMAL) {
            ::SetThreadPriority(handle, THREAD_MODE_BACKGROUND_END);
        }

        // reduce CPU, page and IO priority for the current thread
        if (!::SetThreadPriority(handle, THREAD_MODE_BACKGROUND_BEGIN)) {
            if (ERROR_THREAD_MODE_ALREADY_BACKGROUND == ::GetLastError()) {
                XAMP_LOG_DEBUG("Already in background mode");
                return;
            }
            XAMP_LOG_DEBUG("Failed to set begin background mode! error: {}.", GetLastErrorMessage());
            return;
        }

        if (::GetThreadPriority(handle) >= THREAD_PRIORITY_BELOW_NORMAL) {
            if (!::SetThreadPriority(handle, THREAD_PRIORITY_LOWEST)) {
                XAMP_LOG_DEBUG("Failed to set background mode! error: {}.", GetLastErrorMessage());
            }
        }
        break;
    case ThreadPriority::PRIORITY_NORMAL:
        thread_priority = THREAD_PRIORITY_NORMAL;
        break;
    case ThreadPriority::PRIORITY_HIGHEST:
        thread_priority = THREAD_PRIORITY_HIGHEST;
        break;
    }

    if (priority != ThreadPriority::PRIORITY_BACKGROUND) {
        if (!::SetThreadPriority(handle, thread_priority)) {
            XAMP_LOG_DEBUG("Failed to set thread priority! error:{}.", GetLastErrorMessage());
        }
    }
    auto current_priority = ::GetThreadPriority(handle);
    XAMP_LOG_TRACE("Current thread priority is {}.", current_priority);
#else
#ifdef XAMP_OS_LINUX
    sched_param thread_param{};
    if (priority != ThreadPriority::PRIORITY_HIGHEST) {
        const auto error = ::pthread_setschedparam(handle, SCHED_OTHER, &thread_param);
        if (error != 0 && error != EPERM) {
            XAMP_LOG_DEBUG("Failed to set SCHED_OTHER thread priority: {}.", std::strerror(error));
        }
        return;
    }

    const auto min_priority = ::sched_get_priority_min(SCHED_RR);
    const auto max_priority = ::sched_get_priority_max(SCHED_RR);
    if (min_priority < 0 || max_priority < 0) {
        XAMP_LOG_DEBUG("Failed to query SCHED_RR priority range: {}.", std::strerror(errno));
        return;
    }

    thread_param.sched_priority = (std::min)(min_priority + 4, max_priority);
    const auto error = ::pthread_setschedparam(handle, SCHED_RR, &thread_param);
    if (error == EPERM) {
        XAMP_LOG_DEBUG("SCHED_RR thread priority unavailable. Grant CAP_SYS_NICE or rtprio to enable it.");
        return;
    }
    if (error != 0) {
        XAMP_LOG_DEBUG("Failed to set SCHED_RR thread priority: {}.", std::strerror(error));
        return;
    }
    XAMP_LOG_TRACE("Current thread SCHED_RR priority is {}.", thread_param.sched_priority);
#else
    (void)handle;
    (void)priority;
#endif
#endif
}

void SetCurrentThreadPriority(ThreadPriority priority) {
#ifdef XAMP_OS_WIN
    std::thread::native_handle_type current_thread = ::GetCurrentThread();
#else
    std::thread::native_handle_type current_thread = ::pthread_self();
#endif
    SetThreadPriority(current_thread, priority);
}

void SetThreadPriority(std::jthread& thread, ThreadPriority priority) {
	SetThreadPriority(thread.native_handle(), priority);
}

std::string GetCurrentThreadId() {
    std::ostringstream ostr;
    ostr << std::this_thread::get_id();
    return ostr.str();
}

bool IsDebuging() {
#ifdef _DEBUG
    return true;
#else
#ifdef XAMP_OS_WIN
    return ::IsDebuggerPresent();
#else
    return true;
#endif
#endif
}

#ifdef XAMP_OS_WIN

void SetCurrentProcessPriority(ProcessPriority priority) {
    const WinHandle handle(::GetCurrentProcess());
    SetProcessPriority(handle, priority);
}

void SetProcessPriority(int32_t pid, ProcessPriority priority) {
    const WinHandle handle(::OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid));
    SetProcessPriority(handle, priority);
}

bool ExtendProcessWorkingSetSize(size_t size) {
    SIZE_T minimum = 0;
    SIZE_T maximum = 0;

    const auto current_process = ::GetCurrentProcess();

    if (!::GetProcessWorkingSetSize(current_process, &minimum, &maximum)) {
        XAMP_LOG_DEBUG("GetProcessWorkingSetSize return failure! error:{}.", GetLastErrorMessage());
        return false;
    }   

    minimum = SaturatingAdd(minimum, size);
    maximum = std::max(maximum, minimum);
    return ::SetProcessWorkingSetSize(current_process, minimum, maximum);
}

bool EnablePrivilege(std::string_view privilege, bool enable) {
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

bool SetProcessWorkingSetSize(size_t working_set_size) {
    if (!EnablePrivilege("SeLockMemoryPrivilege", true)) {
        return false;
    }
    if (!ExtendProcessWorkingSetSize(working_set_size)) {
        XAMP_LOG_DEBUG("ExtendProcessWorkingSetSize return failure! error:{}.", GetLastErrorMessage());
        return false;
    }
    XAMP_LOG_TRACE("InitWorkingSetSize {} success.", String::FormatBytes(working_set_size));
    return true;
}

void SetCurrentThreadMitigation() {
    PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamic_code_policy{};
    dynamic_code_policy.ProhibitDynamicCode = true;
    dynamic_code_policy.AllowThreadOptOut = true;
    if (!::SetProcessMitigationPolicy(ProcessDynamicCodePolicy, &dynamic_code_policy,
        sizeof(dynamic_code_policy))) {
        XAMP_LOG_DEBUG("Failed to set ProcessDynamicCodePolicy ({}).", GetLastErrorMessage());
    }

    DWORD thread_policy = THREAD_DYNAMIC_CODE_ALLOW;
    if (!::GetThreadInformation(::GetCurrentThread(), ThreadDynamicCodePolicy,
        &thread_policy, sizeof(thread_policy))) {
        XAMP_LOG_DEBUG("Failed to set GetThreadInformation ({})", GetLastErrorMessage());
    }
    if (thread_policy == THREAD_DYNAMIC_CODE_ALLOW) {
        return;
    }
    thread_policy = THREAD_DYNAMIC_CODE_ALLOW;
    if (!::SetThreadInformation(::GetCurrentThread(), ThreadDynamicCodePolicy,
        &thread_policy, sizeof(thread_policy))) {
        XAMP_LOG_DEBUG("Failed to set SetThreadInformation ({})", GetLastErrorMessage());
    }
}

void SetProcessMitigation() {
    PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY signature_policy{};
    signature_policy.MicrosoftSignedOnly = true;
    if (!::SetProcessMitigationPolicy(ProcessSignaturePolicy, &signature_policy,
        sizeof(signature_policy))) {
        XAMP_LOG_DEBUG("Failed to set ProcessSignaturePolicy ({})", GetLastErrorMessage());
    }       
    
    PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY strict_handle_check_policy = {};
    strict_handle_check_policy.HandleExceptionsPermanentlyEnabled = true;
	strict_handle_check_policy.RaiseExceptionOnInvalidHandleReference = true;
    if (!::SetProcessMitigationPolicy(ProcessStrictHandleCheckPolicy, &strict_handle_check_policy,
        sizeof(strict_handle_check_policy))) {
        XAMP_LOG_DEBUG("Failed to set ProcessStrictHandleCheckPolicy ({}).", GetLastErrorMessage());
    }
    
    PROCESS_MITIGATION_ASLR_POLICY mitigation_aslr_policy = {};
    mitigation_aslr_policy.EnableForceRelocateImages = true;
    mitigation_aslr_policy.DisallowStrippedImages = true;
    mitigation_aslr_policy.EnableBottomUpRandomization = true;
    mitigation_aslr_policy.EnableHighEntropy = true;
    if (!::SetProcessMitigationPolicy(ProcessASLRPolicy, &mitigation_aslr_policy,
        sizeof(mitigation_aslr_policy))) {
        XAMP_LOG_DEBUG("Failed to set ProcessASLRPolicy ({}).", GetLastErrorMessage());
    }
    
    PROCESS_MITIGATION_IMAGE_LOAD_POLICY mitigation_image_load_policy = {};
    mitigation_image_load_policy.NoRemoteImages = true;
    mitigation_image_load_policy.NoLowMandatoryLabelImages = true;
    if (!::SetProcessMitigationPolicy(ProcessImageLoadPolicy, &mitigation_image_load_policy,
        sizeof(mitigation_image_load_policy))) {
        XAMP_LOG_DEBUG("Failed to set ProcessImageLoadPolicy ({}).", GetLastErrorMessage());
    }

    PROCESS_MITIGATION_FONT_DISABLE_POLICY font_disable_policy = {};
    font_disable_policy.DisableNonSystemFonts = true;
    if (!::SetProcessMitigationPolicy(ProcessFontDisablePolicy, &font_disable_policy,
        sizeof(font_disable_policy))) {
        XAMP_LOG_DEBUG("Failed to set ProcessFontDisablePolicy ({}).", GetLastErrorMessage());
    }
}
#endif

#define WORKING_SET_LOCKER SharedSingleton<WorkingSetLocker>::getInstance()

bool VirtualMemoryLock(void* address, size_t size) {
    if (size == 0) {
        return true;
    }
    if (address == nullptr) {
        return false;
    }

#ifdef XAMP_OS_WIN
    if (!::VirtualLock(address, size)) { // try lock memory!
        if (!WORKING_SET_LOCKER.reserveForLock(size)) {
            return false;
        }
        if (!::VirtualLock(address, size)) {
            WORKING_SET_LOCKER.releaseLocked(size);
            return false;
        }
        return true;
    }
    WORKING_SET_LOCKER.recordLocked(size);
    return true;
#else
    return ::mlock(address, size) != -1;
#endif
}

bool VirtualMemoryUnLock(void* address, size_t size) {
    if (size == 0) {
        return true;
    }
    if (address == nullptr) {
        return false;
    }

#ifdef XAMP_OS_WIN
    if (!::VirtualUnlock(address, size)) {
        const auto last_error = ::GetLastError();
        if (last_error == ERROR_NOT_LOCKED) {
            WORKING_SET_LOCKER.releaseLocked(size);
        }
        ::SetLastError(last_error);
        return false;
    }
    WORKING_SET_LOCKER.releaseLocked(size);
    return true;
#else
    return ::munlock(address, size) != -1;
#endif
}

std::string GetSequentialUUID() {
#ifdef XAMP_OS_WIN
    UUID uuid{};
    std::string result;
    RPC_STATUS status = ::UuidCreateSequential(&uuid);
    if (status == RPC_S_OK) {
        RPC_CSTR uuid_string;
        ::UuidToStringA(&uuid, &uuid_string);
        result.assign(reinterpret_cast<const char*>(uuid_string));
        ::RpcStringFreeA(&uuid_string);
        String::Remove(result, "-");
    }
    return result;
#else
    uuid_t uuid{};
    char uuid_string[37]{};
    ::uuid_generate_time(uuid);
    ::uuid_unparse_lower(uuid, uuid_string);
    std::string result(uuid_string);
    String::Remove(result, "-");
    return result;
#endif
}

void MSleep(std::chrono::milliseconds timeout) {
    WaitableTimer timer;
    timer.setTimeout(timeout);
    timer.Wait();
}

uint64_t GetSystemEntropy() {
    const auto r0{ (GenRandomSeed()) };
    const auto r1{ (GenRandomSeed()) };
    return (r1 << 32) | (r0 & UINT64_C(0xffffffff));
}

uint64_t GenRandomSeed() {
    uint64_t seed = 0;
#ifdef XAMP_OS_WIN
    struct BCryptContextTraits final {
        static BCRYPT_ALG_HANDLE invalid() {
            return nullptr;
        }

        static void close(BCRYPT_ALG_HANDLE value) {
            ::BCryptCloseAlgorithmProvider(value, 0);
        }
    };

    using BCryptContext = UniqueHandle<BCRYPT_ALG_HANDLE, BCryptContextTraits>;
    BCRYPT_ALG_HANDLE prov = nullptr;

    if (!BCRYPT_SUCCESS(::BCryptOpenAlgorithmProvider(&prov, BCRYPT_RNG_ALGORITHM, nullptr, 0))) {
        return std::random_device{}();
    }

    const BCryptContext context(prov);
    if (!BCRYPT_SUCCESS(::BCryptGenRandom(context.get(),
        reinterpret_cast<PUCHAR>(&seed),
        sizeof(seed),
        0))) {
        return std::random_device{}();
    }
#else
    seed = std::random_device{}();
#endif
    return seed;
}

void CpuRelax() {
#ifdef XAMP_OS_WIN
    YieldProcessor();
#else
    __asm__ __volatile__("pause");
#endif
}

void Assert(const char* message, const char* file_, uint32_t line) {
    XAMP_LOG_DEBUG("ASSERT failure: {} file: {}:{}", message, file_, line);
#ifdef XAMP_OS_WIN
    const auto utf16_message = String::ToStdWString(message);
    const auto utf16_file_name = String::ToStdWString(file_);
    _wassert(utf16_message.c_str(), utf16_file_name.c_str(), line);
#endif
}

XAMP_BASE_NAMESPACE_END
