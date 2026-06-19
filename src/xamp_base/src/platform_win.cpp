#include <base/platform_win.h>

#ifdef XAMP_OS_WIN

#include <base/assert.h>
#include <base/dll.h>
#include <base/logger.h>
#include <base/memory.h>
#include <base/platfrom_handle.h>
#include <base/shared_singleton.h>
#include <base/str_utilts.h>

#include <rpc.h>
#include <rpcnterr.h>
#include <wincrypt.h>

#include <algorithm>
#include <crtdbg.h>
#include <limits>
#include <mutex>
#include <random>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
    SIZE_T saturatingAdd(SIZE_T lhs, size_t rhs) {
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
            active_locked_bytes_ = saturatingAdd(active_locked_bytes_, size);
            XAMP_LOG_DEBUG("VirtualLock succeeded without working set resize. locked: {} active_locked: {}.",
                String::formatBytes(size),
                String::formatBytes(active_locked_bytes_));
        }

        bool reserveForLock(size_t size) {
            if (size == 0) {
                return true;
            }

            std::lock_guard lock{ mutex_ };
            if (!ensureInitialized()) {
                return false;
            }

            const auto requested_locked_bytes = saturatingAdd(active_locked_bytes_, size);
            const auto target_minimum = saturatingAdd(initial_minimum_, requested_locked_bytes);
            const auto target_maximum = (std::max)(initial_maximum_, target_minimum);

            if (requested_minimum_ < target_minimum || requested_maximum_ < target_maximum) {
                const auto current_process = ::GetCurrentProcess();
                if (!::SetProcessWorkingSetSize(current_process, target_minimum, target_maximum)) {
                    XAMP_LOG_DEBUG(
                        "SetProcessWorkingSetSize failed. locked: {} active_locked: {} target_minimum: {} target_maximum: {} error:{}.",
                        String::formatBytes(size),
                        String::formatBytes(active_locked_bytes_),
                        String::formatBytes(target_minimum),
                        String::formatBytes(target_maximum),
                        GetLastErrorMessage());
                    return false;
                }
                requested_minimum_ = target_minimum;
                requested_maximum_ = target_maximum;
                XAMP_LOG_DEBUG(
                    "SetProcessWorkingSetSize succeeded. locked: {} active_locked: {} minimum: {} maximum: {}.",
                    String::formatBytes(size),
                    String::formatBytes(requested_locked_bytes),
                    String::formatBytes(requested_minimum_),
                    String::formatBytes(requested_maximum_));
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
                String::formatBytes(size),
                String::formatBytes(active_locked_bytes_));
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
                String::formatBytes(initial_minimum_),
                String::formatBytes(initial_maximum_));
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

    void setProcessPriority(const WinHandle& handle, ProcessPriority priority) {
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
            (priority == ProcessPriority::PRIORITY_BACKGROUND) && enable_eco_qos
            ? PROCESS_POWER_THROTTLING_EXECUTION_SPEED
            : 0;
        if (!::SetProcessInformation(handle.get(), ProcessPowerThrottling, &power_throttling, sizeof(power_throttling))) {
            XAMP_LOG_DEBUG("Failed to set SetProcessInformation! error: {}.", GetLastErrorMessage());
        }
    }

    void setNativeThreadPriority(std::thread::native_handle_type handle, ThreadPriority priority) {
        auto thread_priority = THREAD_PRIORITY_NORMAL;
        switch (priority) {
        case ThreadPriority::PRIORITY_BACKGROUND:
            if (::GetThreadPriority(handle) >= THREAD_PRIORITY_BELOW_NORMAL) {
                ::SetThreadPriority(handle, THREAD_MODE_BACKGROUND_END);
            }

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
    }
}

bool atomicWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, uint32_t milliseconds) {
    return ::WaitOnAddress(&to_wait_on, &expected, sizeof(expected), milliseconds) != 0;
}

void atomicWakeSingle(std::atomic<uint32_t>& to_wake) {
    ::WakeByAddressSingle(&to_wake);
}

void atomicWakeAll(std::atomic<uint32_t>& to_wake) {
    ::WakeByAddressAll(&to_wake);
}

void setThreadName(std::wstring const& name) {
    const WinHandle thread(::GetCurrentThread());
    ::SetThreadDescription(thread.get(), name.c_str());
}

void setCurrentThreadPriority(ThreadPriority priority) {
    setNativeThreadPriority(::GetCurrentThread(), priority);
}

void setThreadPriority(std::jthread& thread, ThreadPriority priority) {
    setNativeThreadPriority(thread.native_handle(), priority);
}

bool isDebugging() {
#ifdef _DEBUG
    return true;
#else
    return ::IsDebuggerPresent();
#endif
}

void setCurrentProcessPriority(ProcessPriority priority) {
    const WinHandle handle(::GetCurrentProcess());
    setProcessPriority(handle, priority);
}

void setProcessPriority(int32_t pid, ProcessPriority priority) {
    const WinHandle handle(::OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid));
    setProcessPriority(handle, priority);
}

bool extendProcessWorkingSetSize(size_t size) {
    SIZE_T minimum = 0;
    SIZE_T maximum = 0;

    const auto current_process = ::GetCurrentProcess();

    if (!::GetProcessWorkingSetSize(current_process, &minimum, &maximum)) {
        XAMP_LOG_DEBUG("GetProcessWorkingSetSize return failure! error:{}.", GetLastErrorMessage());
        return false;
    }

    minimum = saturatingAdd(minimum, size);
    maximum = (std::max)(maximum, minimum);
    return ::SetProcessWorkingSetSize(current_process, minimum, maximum);
}

bool enablePrivilege(std::string_view privilege, bool enable) {
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

bool setProcessWorkingSetSize(size_t working_set_size) {
    if (!enablePrivilege("SeLockMemoryPrivilege", true)) {
        return false;
    }
    if (!extendProcessWorkingSetSize(working_set_size)) {
        XAMP_LOG_DEBUG("extendProcessWorkingSetSize return failure! error:{}.", GetLastErrorMessage());
        return false;
    }
    XAMP_LOG_TRACE("InitWorkingSetSize {} success.", String::formatBytes(working_set_size));
    return true;
}

void setCurrentThreadMitigation() {
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

void setProcessMitigation() {
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

bool virtualMemoryLock(void* address, size_t size) {
    if (size == 0) {
        return true;
    }
    if (address == nullptr) {
        return false;
    }

    auto& locker = SharedSingleton<WorkingSetLocker>::getInstance();
    if (!::VirtualLock(address, size)) {
        if (!locker.reserveForLock(size)) {
            return false;
        }
        if (!::VirtualLock(address, size)) {
            locker.releaseLocked(size);
            return false;
        }
        return true;
    }
    locker.recordLocked(size);
    return true;
}

bool virtualMemoryUnlock(void* address, size_t size) {
    if (size == 0) {
        return true;
    }
    if (address == nullptr) {
        return false;
    }

    auto& locker = SharedSingleton<WorkingSetLocker>::getInstance();
    if (!::VirtualUnlock(address, size)) {
        const auto last_error = ::GetLastError();
        if (last_error == ERROR_NOT_LOCKED) {
            locker.releaseLocked(size);
        }
        ::SetLastError(last_error);
        return false;
    }
    locker.releaseLocked(size);
    return true;
}

uint64_t genRandomSeed() {
    struct BCryptContextTraits final {
        static BCRYPT_ALG_HANDLE invalid() {
            return nullptr;
        }

        static void close(BCRYPT_ALG_HANDLE value) {
            ::BCryptCloseAlgorithmProvider(value, 0);
        }
    };

    using BCryptContext = UniqueHandle<BCRYPT_ALG_HANDLE, BCryptContextTraits>;
    uint64_t seed = 0;
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
    return seed;
}

void cpuRelax() {
    YieldProcessor();
}
void assertFailed(const char* message, const char* file_, uint32_t line) {
    XAMP_LOG_DEBUG("ASSERT failure: {} file: {}:{}", message, file_, line);
    const auto utf16_message = String::toStdWString(message);
    const auto utf16_file_name = String::toStdWString(file_);
    _wassert(utf16_message.c_str(), utf16_file_name.c_str(), line);
}

std::string getSequentialUuid() {
    UUID uuid{};
    std::string result;
    RPC_STATUS status = ::UuidCreateSequential(&uuid);
    if (status == RPC_S_OK) {
        RPC_CSTR uuid_string;
        ::UuidToStringA(&uuid, &uuid_string);
        result.assign(reinterpret_cast<const char*>(uuid_string));
        ::RpcStringFreeA(&uuid_string);
        String::remove(result, "-");
    }
    return result;
}

XAMP_BASE_NAMESPACE_END

#endif
