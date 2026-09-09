#include <base/platform_linux.h>

#ifdef XAMP_OS_LINUX

#include <base/logger.h>
#include <base/str_utilts.h>

#include <algorithm>

#include <stdlib.h>
#include <cerrno>
#include <climits>
#include <cstring>
#include <linux/futex.h>
#include <random>
#include <sched.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <uuid/uuid.h>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
    void setNativeThreadPriority(std::thread::native_handle_type handle, ThreadPriority priority) {
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
    }
}

bool atomicWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, uint32_t milliseconds) {
    static_assert(sizeof(std::atomic<uint32_t>) == sizeof(uint32_t),
        "std::atomic<uint32_t> must have the same layout size as uint32_t for futex wait.");
    static_assert(std::atomic<uint32_t>::is_always_lock_free,
        "std::atomic<uint32_t> must be lock-free for futex wait.");

    auto* futex_address = reinterpret_cast<uint32_t*>(&to_wait_on);
    const auto wait_forever = milliseconds == kInfinity;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);

    for (;;) {
        if (to_wait_on.load(std::memory_order_acquire) != expected) {
            return true;
        }

        timespec timeout{};
        timespec* timeout_ptr = nullptr;
        if (!wait_forever) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                return false;
            }

            const auto remaining = std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - now);
            const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(remaining);
            timeout.tv_sec = seconds.count();
            timeout.tv_nsec = static_cast<long>((remaining - seconds).count());
            timeout_ptr = &timeout;
        }

        const auto result = ::syscall(
            SYS_futex,
            futex_address,
            FUTEX_WAIT_PRIVATE,
            expected,
            timeout_ptr,
            nullptr,
            0);
        if (result == 0) {
            continue;
        }

        switch (errno) {
        case EAGAIN:
            return true;
        case EINTR:
            continue;
        case ETIMEDOUT:
            return to_wait_on.load(std::memory_order_acquire) != expected;
        default:
            return false;
        }
    }
}

void atomicWakeSingle(std::atomic<uint32_t>& to_wake) {
    ::syscall(
        SYS_futex,
        reinterpret_cast<uint32_t*>(&to_wake),
        FUTEX_WAKE_PRIVATE,
        1,
        nullptr,
        nullptr,
        0);
}

void atomicWakeAll(std::atomic<uint32_t>& to_wake) {
    ::syscall(
        SYS_futex,
        reinterpret_cast<uint32_t*>(&to_wake),
        FUTEX_WAKE_PRIVATE,
        INT_MAX,
        nullptr,
        nullptr,
        0);
}

void setThreadName(std::wstring const& name) {
    static constexpr int kMaxNameLength = 15;
    const auto shortened_name = String::toUtf8String(name).substr(0, kMaxNameLength);
    ::pthread_setname_np(::pthread_self(), shortened_name.c_str());
}

void setThreadPriority(std::jthread& thread, ThreadPriority priority) {
    setNativeThreadPriority(thread.native_handle(), priority);
}

void setCurrentThreadPriority(ThreadPriority priority) {
    setNativeThreadPriority(::pthread_self(), priority);
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
    uint64_t seed = 0;
    ::arc4random_buf(&seed, sizeof(seed));
    return seed;
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
