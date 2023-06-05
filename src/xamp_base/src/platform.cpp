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

#include <thread>
#include <set>

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

static XAMP_ALWAYS_INLINE uint32_t ToMilliseconds(const timespec* ts) noexcept {
    return ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
}

XAMP_BASE_NAMESPACE_BEGIN

/*
* Futex wait implementation.
* 
* @param[out] to_wait_on The atomic variable to wait on.
* @param[in] expected The expected value of the atomic variable.
* @param[in] milliseconds The number of milliseconds to wait for.
* @return true if the atomic variable was woken up, false if the wait timed out.
*/
template <typename T>
static XAMP_ALWAYS_INLINE bool PlatformFutexWait(std::atomic<T>& to_wait_on, uint32_t &expected, uint32_t milliseconds) noexcept {
#ifdef XAMP_OS_WIN    
    return ::WaitOnAddress(&to_wait_on, &expected, sizeof(expected), milliseconds);
#elif defined(XAMP_OS_MAC)
    return ::__ulock_wait(UL_COMPARE_AND_WAIT, &to_wait_on, expected, milliseconds * 1000) >= 0;
#endif
}

/*
* Futex wake implementation.
* 
* @param[out] to_wake The atomic variable to wake up.
*/
template <typename T>
static XAMP_ALWAYS_INLINE void PlatformFutexWakeSingle(std::atomic<T>& to_wake) noexcept {
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
static XAMP_ALWAYS_INLINE void PlatformFutexWakeAll(std::atomic<T>& to_wake) noexcept {
#ifdef XAMP_OS_WIN
    ::WakeByAddressAll(&to_wake);
#elif defined (XAMP_OS_MAC)
    MacOSFutexWake(to_wake, false);
#endif
}

void AtomicWakeSingle(std::atomic<uint32_t>& to_wake) noexcept {
    PlatformFutexWakeSingle(to_wake);
}

void AtomicWakeAll(std::atomic<uint32_t>& to_wake) noexcept {
    PlatformFutexWakeAll(to_wake);
}

bool AtomicWait(std::atomic<uint32_t>& to_wait_on, uint32_t &expected, uint32_t milliseconds) noexcept {
    return PlatformFutexWait(to_wait_on, expected, milliseconds);
}

int32_t AtomicWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, const timespec* to) noexcept {   
    if (to == nullptr) {
        // Wait forever.
        AtomicWait(to_wait_on, expected, kInfinity);
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
        PlatformFutexWait(to_wait_on, expected, 2147000000);
        return 0; /* time-out out of range, claim spurious wake-up */
    }

    // Wait for the specified time-out.
    if (!PlatformFutexWait(to_wait_on, expected, ToMilliseconds(to))) {
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

void SetThreadName(std::wstring const& name) noexcept {
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
        cpus.fill(all_cpu_set);
        cpus[only_use_cpu] = true;
    } else {
        cpus.fill(all_cpu_set);
    }
}

void CpuAffinity::SetCpu(int32_t cpu) {
    cpus[cpu] = true;
}

CpuAffinity::operator bool() const noexcept {
    for (int i = 0; i < cpus.size(); ++i) {
        if (cpus[i]) {
            return true;
        }
    }
    return false;
}

void CpuAffinity::SetAffinity(JThread& thread) {
#ifdef XAMP_OS_WIN
    const DWORD group_count = ::GetActiveProcessorGroupCount();
    for (DWORD group_index = 0; group_index < group_count; ++group_index) {
        GROUP_AFFINITY group_affinity = {};
        group_affinity.Group = group_index;

        const DWORD processor_count = ::GetActiveProcessorCount(group_index);
        DWORD current_processor = 0;
        for (DWORD processor_index = 0; processor_index < processor_count; ++processor_index) {
            if (cpus[group_index * 64 + processor_index]) {
                group_affinity.Mask |= (1ull << processor_index);
                ++current_processor;
			}
        }

        if (!::SetThreadGroupAffinity(thread.native_handle(), &group_affinity, nullptr)) {
            XAMP_LOG_DEBUG("Fail to set SetThreadGroupAffinity");
        }
        else {
            XAMP_LOG_DEBUG("Success to set SetThreadGroupAffinity mask: {:#02X}", group_affinity.Mask);
        }

        for (DWORD processor_index = 0; processor_index < processor_count; ++processor_index) {
            if (cpus[group_index * 64 + processor_index]) {
                PROCESSOR_NUMBER processor_number = {};
                processor_number.Group = group_index;
                processor_number.Number = processor_index;
                processor_number.Reserved = 0;
                if (!::SetThreadIdealProcessorEx(thread.native_handle(), &processor_number, nullptr)) {
                    XAMP_LOG_DEBUG("Fail to set SetThreadIdealProcessorEx");
                }
            }
        }
    }    
#else
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    for (int i = 0; i < cpus.size(); ++i) {
        if (cpus[i]) {
            CPU_SET(i, &cpu_set);
        }
    }
    pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set), &cpu_set);
#endif
}

#ifdef XAMP_OS_WIN
bool EnablePowerThrottling(JThread& thread, bool enable) {
    THREAD_POWER_THROTTLING_STATE throttling_state{ 0 };

    throttling_state.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
    throttling_state.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
    throttling_state.StateMask = enable ? THREAD_POWER_THROTTLING_EXECUTION_SPEED : 0;

    return ::SetThreadInformation(thread.native_handle(), ThreadPowerThrottling,
        &throttling_state,
        sizeof(throttling_state));
}
#endif

void SetThreadPriority(JThread& thread, ThreadPriority priority) noexcept {
#ifdef XAMP_OS_WIN
    auto thread_priority = THREAD_PRIORITY_NORMAL;
    switch (priority) {
    case ThreadPriority::BACKGROUND:
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
    case ThreadPriority::NORMAL:
        thread_priority = THREAD_PRIORITY_NORMAL;
        break;
    case ThreadPriority::HIGHEST:
        thread_priority = THREAD_PRIORITY_HIGHEST;
        break;
    }

    if (priority != ThreadPriority::BACKGROUND) {
        if (!::SetThreadPriority(thread.native_handle(), thread_priority)) {
            XAMP_LOG_DEBUG("Failed to set thread priority! error:{}.", GetLastErrorMessage());
        }
        return;
    }

    auto current_priority = ::GetThreadPriority(thread.native_handle());
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
    ::pthread_setschedparam(thread.native_handle(), SCHED_RR, &thread_param);
#endif
}

PlatformUUID ParseUuidString(const std::string &str) {
    PlatformUUID result{};
#ifdef XAMP_OS_WIN
    UUID uuid;
    auto status = ::UuidFromStringA(RPC_CSTR(str.c_str()), &uuid);
    XAMP_ASSERT(status == RPC_S_OK);
    MemoryCopy(&result, &uuid, sizeof(PlatformUUID));
#else
    uuid_string_t ustr{ 0 };
    MemoryCopy(ustr, str.c_str(), sizeof(ustr));
    uuid_t uuid{ 0 };
    ::uuid_parse(ustr, uuid);
    MemoryCopy(&result, &uuid, sizeof(PlatformUUID));
#endif
    return result;
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
void RedirectStdOut() {
    if (!::AllocConsole()) {
        return;
    }

    auto stdout_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi{};
    if (::GetConsoleScreenBufferInfo(stdout_handle, &csbi)) {
        csbi.dwSize.Y = 480;
        csbi.dwSize.X = 640;
        ::SetConsoleScreenBufferSize(stdout_handle, csbi.dwSize);
    }

    // Redirect "stdin" to the console window.
    if (!freopen("CONIN$", "w", stdin)) 
        return;

    // Redirect "stderr" to the console window.
    if (!freopen("CONOUT$", "w", stderr)) 
        return;

    // Redirect "stdout" to the console window.
    if (!freopen("CONOUT$", "w", stdout))
        return;

    // Turn off buffering for "stdout" ("stderr" is unbuffered by default).
    setbuf(stdout, nullptr);
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
    XAMP_LOG_DEBUG("InitWorkingSetSize {} success.", String::FormatBytes(working_set_size));
    return false;
}

size_t GetAvailablePhysicalMemory() {
    MEMORYSTATUSEX memory_status{};
    memory_status.dwLength = sizeof(memory_status);
    ::GlobalMemoryStatusEx(&memory_status);
    return memory_status.ullAvailPhys;;
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

void MSleep(std::chrono::milliseconds timeout) {
    WaitableTimer timer;
    timer.SetTimeout(timeout);
    timer.Wait();
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

void ExecutionStopwatch::Start() {
    is_running_ = true;
    start_timestamp_ = GetThreadTimes();
}

void ExecutionStopwatch::Stop() {
    is_running_ = false;
    end_timestamp_ = GetThreadTimes();
}

void ExecutionStopwatch::Reset() {
    start_timestamp_ = 0;
    end_timestamp_ = 0;
}

std::chrono::milliseconds ExecutionStopwatch::Elapsed() const {
    int64_t elapsed = end_timestamp_ - start_timestamp_;
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(elapsed));
}

bool ExecutionStopwatch::IsRunning() const noexcept {
    return is_running_;
}

namespace detail {
    static void GetThreadTimes(ULARGE_INTEGER& kernel_time_value, ULARGE_INTEGER user_time_value) {
        FILETIME creation_time, exit_time, kernel_time, user_time;
        if (!::GetThreadTimes(::GetCurrentThread(), &creation_time, &exit_time, &kernel_time, &user_time))
            throw std::runtime_error("Failed to get thread times.");

        kernel_time_value.HighPart = kernel_time.dwHighDateTime;
        kernel_time_value.LowPart = kernel_time.dwLowDateTime;
        user_time_value.HighPart = user_time.dwHighDateTime;
        user_time_value.LowPart = user_time.dwLowDateTime;
    }
}

int64_t ExecutionStopwatch::GetThreadTimes() {
    ULARGE_INTEGER kernel_time_value{}, user_time_value{};
    detail::GetThreadTimes(kernel_time_value, user_time_value);

    return kernel_time_value.QuadPart + user_time_value.QuadPart;
}

double ExecutionStopwatch::GetCpuUsage() const {    
    std::chrono::milliseconds elapsed = Elapsed();
    double elapsed_seconds = elapsed.count() / 1000.0;

    if (elapsed_seconds == 0) {
        return 0;
    }

    ULARGE_INTEGER kernel_time_value{}, user_time_value{};
    detail::GetThreadTimes(kernel_time_value, user_time_value);

    double kernel_time_seconds = kernel_time_value.QuadPart / 10000000.0;  // 锣传计
    double user_time_seconds = user_time_value.QuadPart / 10000000.0;  // 锣传计    
    
    // 璸衡 CPU ㄏノ瞯
    double cpu_usage = (kernel_time_seconds + user_time_seconds) / elapsed_seconds;

    return cpu_usage;
}

XAMP_BASE_NAMESPACE_END
