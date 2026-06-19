#include <base/platform_mac.h>

#ifdef XAMP_OS_MAC

#include <base/logger.h>
#include <base/str_utilts.h>

#include <pthread.h>
#include <random>
#include <sys/mman.h>
#include <uuid/uuid.h>

extern "C" int __ulock_wait(uint32_t operation, void* addr, uint64_t value,
    uint32_t timeout);
extern "C" int __ulock_wake(uint32_t operation, void* addr, uint64_t wake_value);

#define UL_COMPARE_AND_WAIT 1
#define ULF_WAKE_ALL 0x00000100

XAMP_BASE_NAMESPACE_BEGIN

namespace {
    template <typename T>
    int macOSFutexWake(std::atomic<T>& to_wake, bool notify_one) {
        return ::__ulock_wake(UL_COMPARE_AND_WAIT | (notify_one ? 0 : ULF_WAKE_ALL), &to_wake, 0);
    }
}

bool atomicWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, uint32_t milliseconds) {
    const auto timeout_us = milliseconds == kInfinity ? 0 : milliseconds * 1000;
    return ::__ulock_wait(UL_COMPARE_AND_WAIT, &to_wait_on, expected, timeout_us) == 0;
}

void atomicWakeSingle(std::atomic<uint32_t>& to_wake) {
    macOSFutexWake(to_wake, true);
}

void atomicWakeAll(std::atomic<uint32_t>& to_wake) {
    macOSFutexWake(to_wake, false);
}

void setThreadName(std::wstring const& name) {
    static constexpr int kMaxNameLength = 63;
    const auto shortened_name = String::toUtf8String(name).substr(0, kMaxNameLength);
    ::pthread_setname_np(shortened_name.c_str());
}

void setThreadPriority(std::jthread& thread, ThreadPriority priority) {
    (void)thread;
    (void)priority;
}

void setCurrentThreadPriority(ThreadPriority priority) {
    (void)priority;
}

bool isDebugging() {
#ifdef _DEBUG
    return true;
#else
    return true;
#endif
}

bool virtualMemoryLock(void* address, size_t size) {
    if (size == 0) {
        return true;
    }
    if (address == nullptr) {
        return false;
    }
    return ::mlock(address, size) != -1;
}

bool virtualMemoryUnlock(void* address, size_t size) {
    if (size == 0) {
        return true;
    }
    if (address == nullptr) {
        return false;
    }
    return ::munlock(address, size) != -1;
}

uint64_t genRandomSeed() {
    return std::random_device{}();
}

void cpuRelax() {
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("pause");
#else
    std::this_thread::yield();
#endif
}
void assertFailed(const char* message, const char* file_, uint32_t line) {
    XAMP_LOG_DEBUG("ASSERT failure: {} file: {}:{}", message, file_, line);
}

std::string getSequentialUuid() {
    uuid_t uuid{};
    char uuid_string[37]{};
    ::uuid_generate_time(uuid);
    ::uuid_unparse_lower(uuid, uuid_string);
    std::string result(uuid_string);
    String::remove(result, "-");
    return result;
}

XAMP_BASE_NAMESPACE_END

#endif
