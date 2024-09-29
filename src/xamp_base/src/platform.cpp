#include <base/platform.h>

#include <base/dll.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/rng.h>
#include <base/assert.h>
#include <base/waitabletimer.h>
#include <base/exception.h>
#include <base/memory.h>
#include <base/platfrom_handle.h>

#ifdef XAMP_OS_WIN
#include <rpcnterr.h>
#include <rpc.h>
#include <wincrypt.h>
#else
#include <uuid/uuid.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#include <sys/mman.h>
#endif

#include <bitset>
#include <thread>

#ifdef XAMP_OS_MAC
extern "C" int __ulock_wait(uint32_t operation, void* addr, uint64_t value,
                            uint32_t timeout); /* timeout is specified in microseconds */
extern "C" int __ulock_wake(uint32_t operation, void* addr, uint64_t wake_value);

#define UL_COMPARE_AND_WAIT	1
#define ULF_WAKE_ALL 0x00000100

template <typename T>
static int MacOSFutexWake(std::atomic<T>& to_wake, bool notify_one) noexcept {
    return ::__ulock_wake(UL_COMPARE_AND_WAIT | (notify_one ? 0 : ULF_WAKE_ALL), &to_wake, 0);
}
#endif

XAMP_BASE_NAMESPACE_BEGIN

namespace {
#ifdef XAMP_OS_WIN
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

    uint64_t ToMilliseconds(const timespec* ts) noexcept {
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
    template <typename T>
    bool PlatformFutexWait(std::atomic<T>& to_wait_on, uint32_t& expected, uint32_t milliseconds) noexcept {
#ifdef XAMP_OS_WIN
        // 在 Windows 上，INFINITE 通常定義為 0xFFFFFFFF，表示無限等待
        return ::WaitOnAddress(&to_wait_on, &expected, sizeof(expected), milliseconds) != 0;
#elif defined(XAMP_OS_MAC)
        // 在 macOS 上，超時為 0 表示無限等待，時間單位為微秒
        uint32_t timeout_us = (milliseconds == kInfinity) ? 0 : milliseconds * 1000;
        return ::__ulock_wait(UL_COMPARE_AND_WAIT, &to_wait_on, expected, timeout_us) == 0;
#endif
    }

    /*
    * Futex wake implementation.
    *
    * @param[out] to_wake The atomic variable to wake up.
    */
    template <typename T>
    void PlatformFutexWakeSingle(std::atomic<T>& to_wake) noexcept {
#ifdef XAMP_OS_WIN
        ::WakeByAddressSingle(&to_wake);
#elif defined (XAMP_OS_MAC)
        MacOSFutexWake(to_wake, true);
#endif
    }

    /*
    * Futex wake all implementation.
    *
    * @param[out] to_wake The atomic variable to wake up.
    */
    template <typename T>
	void PlatformFutexWakeAll(std::atomic<T>& to_wake) noexcept {
#ifdef XAMP_OS_WIN
        ::WakeByAddressAll(&to_wake);
#elif defined (XAMP_OS_MAC)
        MacOSFutexWake(to_wake, false);
#endif
    }
}

void AtomicWakeSingle(std::atomic<uint32_t>& to_wake) noexcept {
    PlatformFutexWakeSingle(to_wake);
}

void AtomicWakeAll(std::atomic<uint32_t>& to_wake) noexcept {
    PlatformFutexWakeAll(to_wake);
}

bool AtomicWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, uint32_t milliseconds) noexcept {
    return PlatformFutexWait(to_wait_on, expected, milliseconds);
}

int32_t AtomicWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, const timespec* to) noexcept {   
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

#ifndef XAMP_OS_WIN
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

void SetThreadName(std::wstring const& name) {
#ifdef XAMP_OS_WIN
	const WinHandle thread(::GetCurrentThread());
    ::SetThreadDescription(thread.get(), name.c_str());
#else
    // Mac OS X does not expose the length limit of the name, so
    // hardcode it.
    static constexpr int kMaxNameLength = 63;
    const auto shortened_name = String::ToUtf8String(name).substr(0, kMaxNameLength);
    ::pthread_setname_np(shortened_name.c_str());
#endif
}

const CpuAffinity CpuAffinity::kAll(-1, true);
const CpuAffinity CpuAffinity::kInvalid(-1, false);

CpuAffinity::CpuAffinity(int32_t only_use_cpu, bool all_cpu_set) {
    if (only_use_cpu != -1) {
        cpus_.fill(all_cpu_set);
        cpus_[only_use_cpu] = true;
    } else {
        cpus_.fill(all_cpu_set);
    }
}

void CpuAffinity::SetCpu(int32_t cpu) {
    cpus_[cpu] = true;
}

bool CpuAffinity::IsCoreUse(int32_t cpu) const {
    return cpus_[cpu];
}

CpuAffinity::operator bool() const noexcept {
    for (int i = 0; i < cpus_.size(); ++i) {
        if (cpus_[i]) {
            return true;
        }
    }
    return false;
}

size_t CpuAffinity::GetCoreCount() {
#ifdef XAMP_OS_WIN
    SYSTEM_INFO sysinfo{};
    ::GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    return 0;
#endif
}

void CpuAffinity::SetAffinity(JThread& thread) {
#ifdef XAMP_OS_WIN
    const DWORD group_count = ::GetActiveProcessorGroupCount();
    for (DWORD group_index = 0; group_index < group_count; ++group_index) {
        GROUP_AFFINITY group_affinity = {};
        group_affinity.Group = group_index;

        const DWORD processor_count = ::GetActiveProcessorCount(group_index);
        group_affinity.Mask = 0;

        for (DWORD processor_index = 0; processor_index < processor_count; ++processor_index) {
            if (cpus_[group_index * 64 + processor_index]) {
                group_affinity.Mask |= (1ull << processor_index);
            }
        }

        if (!::SetThreadGroupAffinity(thread.native_handle(), &group_affinity, nullptr)) {
            XAMP_LOG_DEBUG("Failed to set SetThreadGroupAffinity! error: {}.", GetLastErrorMessage());
        }
        else {
            XAMP_LOG_TRACE("Success to set SetThreadGroupAffinity mask: {:#02X}", group_affinity.Mask);
        }

        for (DWORD processor_index = 0; processor_index < processor_count; ++processor_index) {
            if (cpus_[group_index * 64 + processor_index]) {
                PROCESSOR_NUMBER processor_number = {};
                processor_number.Group = group_index;
                processor_number.Number = processor_index;
                processor_number.Reserved = 0;
                if (!::SetThreadIdealProcessorEx(thread.native_handle(), &processor_number, nullptr)) {
                    XAMP_LOG_DEBUG("Failed to set SetThreadIdealProcessorEx! error: {}.", GetLastErrorMessage());
                }
                else {
                    XAMP_LOG_TRACE("Success to set SetThreadIdealProcessorEx group: {} processor index: {}", group_index, processor_index);
                    break;
                }
            }
        }
}
#else
	// TODO: Implement for Linux or macOS.
#endif
}

void SetThreadPriority(JThread& thread, ThreadPriority priority) {
#ifdef XAMP_OS_WIN
    auto thread_priority = THREAD_PRIORITY_NORMAL;
    switch (priority) {
    case ThreadPriority::PRIORITY_BACKGROUND:
        if (::GetThreadPriority(thread.native_handle()) >= THREAD_PRIORITY_BELOW_NORMAL) {
            ::SetThreadPriority(thread.native_handle(), THREAD_MODE_BACKGROUND_END);
        }

        // reduce CPU, page and IO priority for the current thread
        if (!::SetThreadPriority(thread.native_handle(), THREAD_MODE_BACKGROUND_BEGIN)) {
            if (ERROR_THREAD_MODE_ALREADY_BACKGROUND == ::GetLastError()) {
                XAMP_LOG_DEBUG("Already in background mode");
                return;
            }
            XAMP_LOG_DEBUG("Failed to set begin background mode! error: {}.", GetLastErrorMessage());
            return;
        }

        if (::GetThreadPriority(thread.native_handle()) >= THREAD_PRIORITY_BELOW_NORMAL) {
            if (!::SetThreadPriority(thread.native_handle(), THREAD_PRIORITY_LOWEST)) {
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
        if (!::SetThreadPriority(thread.native_handle(), thread_priority)) {
            XAMP_LOG_DEBUG("Failed to set thread priority! error:{}.", GetLastErrorMessage());
        }        
    }    
    auto current_priority = ::GetThreadPriority(thread.native_handle());
    XAMP_LOG_TRACE("Current thread priority is {}.", current_priority);
#else
#if !defined(PTHREAD_MIN_PRIORITY)
#define PTHREAD_MIN_PRIORITY  0
#endif
#if !defined(PTHREAD_MAX_PRIORITY)
#define PTHREAD_MAX_PRIORITY 31
#endif

    auto thread_priority = PTHREAD_MIN_PRIORITY;
    switch (priority) {
    case ThreadPriority::PRIORITY_BACKGROUND:
        thread_priority = PTHREAD_MIN_PRIORITY;
        break;
    case ThreadPriority::PRIORITY_NORMAL:
    default:
        thread_priority = PTHREAD_MAX_PRIORITY / 2;
        break;
    case ThreadPriority::PRIORITY_HIGHEST:
        thread_priority = PTHREAD_MAX_PRIORITY;
        break;
    }
    struct sched_param thread_param;
    thread_param.sched_priority = thread_priority;
    ::pthread_setschedparam(thread.native_handle(), SCHED_RR, &thread_param);
#endif
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

bool SetFileLowIoPriority(int32_t handle) {
    FILE_IO_PRIORITY_HINT_INFO priority_hint;
    priority_hint.PriorityHint = IoPriorityHintLow;
    const auto file_handle = reinterpret_cast<HANDLE>(handle);
    return ::SetFileInformationByHandle(file_handle,
        FileIoPriorityHintInfo, 
        &priority_hint, 
        sizeof(priority_hint));
}

void SetThreadMitigation() {
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

bool VirtualMemoryLock(void* address, size_t size) {
#ifdef XAMP_OS_WIN
    if (!::VirtualLock(address, size)) { // try lock memory!
        if (!ExtendProcessWorkingSetSize(size)) {
            throw PlatformException("ExtendProcessWorkingSetSize return failure!");
        }
        if (!::VirtualLock(address, size)) {
            throw PlatformException("VirtualLock return failure!");
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
    return ::munlock(address, size) != -1;
#endif
}

std::string GetSequentialUUID() {
    UUID uuid{};
    std::string result;
    RPC_STATUS status = ::UuidCreateSequential(&uuid);
    if (status == RPC_S_OK) {
        RPC_CSTR uuid_string;
        ::UuidToStringA(&uuid, &uuid_string);
        result.assign(reinterpret_cast<const char*>(uuid_string));
        ::RpcStringFreeA(&uuid_string);
    }
    return result;
}

void MSleep(std::chrono::milliseconds timeout) {
    WaitableTimer timer;
    timer.SetTimeout(timeout);
    timer.Wait();
}

uint64_t GetSystemEntropy() noexcept {
    const auto r0{ (GenRandomSeed()) };
    const auto r1{ (GenRandomSeed()) };
    return (r1 << 32) | (r0 & UINT64_C(0xffffffff));
}

uint64_t GenRandomSeed() noexcept {
    uint64_t seed = 0;
#ifdef XAMP_OS_WIN
    struct BCryptContextTraits final {
        static BCRYPT_ALG_HANDLE invalid() noexcept {
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

void CpuRelax() noexcept {
#ifdef XAMP_OS_WIN
    YieldProcessor();
#else
    __asm__ __volatile__("pause");
#endif
}

void Assert(const char* message, const char* file, uint32_t line) {
    XAMP_LOG_DEBUG("ASSERT failure: {} file: {}:{}", message, file, line);
#ifdef XAMP_OS_WIN
    const auto utf16_message = String::ToStdWString(message);
    const auto utf16_file_name = String::ToStdWString(file);
    _wassert(utf16_message.c_str(), utf16_file_name.c_str(), line);
#endif
}

XAMP_BASE_NAMESPACE_END
