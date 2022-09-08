#include <thread>

#include <base/dll.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/rng.h>
#include <base/platform.h>
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

namespace xamp::base {

template <typename T>
static XAMP_ALWAYS_INLINE bool PlatformFutexWait(std::atomic<T>& to_wait_on, uint32_t &expected, uint32_t milliseconds) noexcept {
#ifdef XAMP_OS_WIN
    return ::WaitOnAddress(&to_wait_on, &expected, sizeof(expected), milliseconds);
#elif defined(XAMP_OS_MAC)
    return ::__ulock_wait(UL_COMPARE_AND_WAIT, &to_wait_on, expected, milliseconds * 1000) >= 0;
#endif
}

template <typename T>
static XAMP_ALWAYS_INLINE void PlatformFutexWakeSingle(std::atomic<T>& to_wake) noexcept {
#ifdef XAMP_OS_WIN
    ::WakeByAddressSingle(&to_wake);
#elif defined (XAMP_OS_MAC)
    MacOSFutexWake(to_wake, true);
#endif
}

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
        AtomicWait(to_wait_on, expected, kInefinity);
        return 0;
    }

    if (to->tv_nsec >= 1000000000) {
        errno = EINVAL;
        return -1;
    }

    if (to->tv_sec >= 2147) {
        PlatformFutexWait(to_wait_on, expected, 2147000000);
        return 0; /* time-out out of range, claim spurious wake-up */
    }

    if (!PlatformFutexWait(to_wait_on, expected, ToMilliseconds(to))) {
        errno = ETIMEDOUT;
        return -1;
    }
    return 0;
}

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

static int32_t FFSLL(uint64_t mask) {
    auto bit = 0;

    if (mask == 0) {
        return 0;
    }

    for (bit = 1; !(mask & 1); bit++) {
        mask = mask >> 1;
    }
    return bit;
}

int32_t CpuAffinity::FirstSetCpu() const {
    auto row_first_cpu = 0;
    auto cpus_offset = 0;
    auto mask_first_cpu = -1;
    auto row = 0u;

    while (mask_first_cpu < 0 && row < XAMP_CPU_MASK_ROWS) {
        row_first_cpu = FFSLL(mask[row]) - 1;
        if (row_first_cpu > -1) {
            mask_first_cpu = cpus_offset + row_first_cpu;
        } else {
            cpus_offset += XAMP_MASK_STRIDE;
            row++;
        }
    }
    return mask_first_cpu;
}

void SetThreadAffinity(std::thread& thread, CpuAffinity affinity) noexcept {
#ifdef XAMP_OS_WIN
    auto cpu = affinity.FirstSetCpu();
    auto group_size = 0;

	const auto groups = ::GetActiveProcessorGroupCount();
    auto cpus_offset = 0, group = -1, number = 0;

    for (DWORD i = 0; i < groups; i++) {
	    group_size = ::GetActiveProcessorCount(i);
        if (cpus_offset + group_size > cpu) {
            group = i;
            number = cpu - cpus_offset;
            break;
        }
        cpus_offset += group_size;
	}

    if (group == -1) {
        XAMP_LOG_DEBUG("Not found any group affinity:{}", affinity);
        return;
    }

    auto group_cpumask = affinity.mask[0];

    GROUP_AFFINITY group_affinity;
    group_affinity.Group = static_cast<WORD>(group);
    group_affinity.Mask = group_cpumask;
    group_affinity.Reserved[0] = 0;
    group_affinity.Reserved[1] = 0;
    group_affinity.Reserved[2] = 0;
    if (!::SetThreadGroupAffinity(thread.native_handle(), &group_affinity, nullptr)) {
        XAMP_LOG_DEBUG("Can't set thread group affinity");
    }

    XAMP_LOG_DEBUG("Thread affinity mask: {:#04x}", group_affinity.Mask);

    PROCESSOR_NUMBER processor_number;
    processor_number.Group = group;
    processor_number.Number = cpu;
    processor_number.Reserved = 0;
    if (!::SetThreadIdealProcessorEx(thread.native_handle(), &processor_number, nullptr)) {
        XAMP_LOG_DEBUG("Can't set threadeal processor");
    }
#else
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

 Path GetTempFilePath() {
	const auto temp_path = Fs::temp_directory_path();
	return temp_path / Fs::path(MakeTempFileName() + ".tmp");
}

std::string MakeTempFileName() {
    std::string filename = "xamp_temp_";
    static constexpr std::string_view kFilenameCharacters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (auto i = 0; i < 8; ++i) {
        filename += kFilenameCharacters[PRNG().Next(kFilenameCharacters.length() - 1)];
    }
    return filename;
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
            throw PlatformSpecException("VirtualLock return failure!");
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

}
