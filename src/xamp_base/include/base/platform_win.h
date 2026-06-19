//====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <base/base.h>
#include <base/enum.h>
#include <base/fs.h>
#include <base/memory.h>

XAMP_BASE_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(
    ThreadPriority,
    PRIORITY_UNKNOWN,
    PRIORITY_BACKGROUND,
    PRIORITY_NORMAL,
    PRIORITY_HIGHEST)

XAMP_MAKE_ENUM(
    ProcessPriority,
    PRIORITY_UNKNOWN,
    PRIORITY_BACKGROUND,
    PRIORITY_BACKGROUND_PERCEIVABLE,
    PRIORITY_FOREGROUND_KEYBOARD,
    PRIORITY_PREALLOC,
    PRIORITY_FOREGROUND,
    PRIORITY_FOREGROUND_HIGH,
    PRIORITY_PARENT_PROCESS
)

inline constexpr uint32_t kInfinity =
#ifdef XAMP_OS_WIN
    0xFFFFFFFF;
#else
    0;
#endif

XAMP_BASE_API void setThreadPriority(std::jthread& thread,
    ThreadPriority priority);

XAMP_BASE_API void setThreadName(std::wstring const& name);

XAMP_BASE_API std::string getCurrentThreadId();

XAMP_BASE_API bool isDebugging();

XAMP_BASE_API void setCurrentThreadPriority(ThreadPriority priority);

XAMP_BASE_API bool virtualMemoryLock(void* address, size_t size);

XAMP_BASE_API bool virtualMemoryUnlock(void* address, size_t size);

XAMP_BASE_API void mSleep(std::chrono::milliseconds timeout);

XAMP_BASE_API int32_t atomicWait(std::atomic<uint32_t>& to_wait_on,
    uint32_t expected,
    const timespec* to);

XAMP_BASE_API bool atomicWait(std::atomic<uint32_t>& to_wait_on,
    uint32_t expected,
    uint32_t milliseconds);

XAMP_BASE_API void atomicWakeSingle(std::atomic<uint32_t>& to_wake);

XAMP_BASE_API void atomicWakeAll(std::atomic<uint32_t>& to_wake);

XAMP_BASE_API uint64_t genRandomSeed();

XAMP_BASE_API uint64_t getSystemEntropy();

XAMP_BASE_API void cpuRelax();

XAMP_BASE_API void assertFailed(const char* message, const char* file_, uint32_t line);

XAMP_BASE_API std::string getSequentialUuid();

XAMP_BASE_NAMESPACE_END
XAMP_BASE_NAMESPACE_BEGIN

XAMP_BASE_API void setCurrentProcessPriority(ProcessPriority priority);

XAMP_BASE_API void setProcessPriority(int32_t pid, ProcessPriority priority);

XAMP_BASE_API bool enablePrivilege(std::string_view privilege, bool enable);

XAMP_BASE_API bool extendProcessWorkingSetSize(size_t size);

XAMP_BASE_API bool setProcessWorkingSetSize(size_t working_set_size);

XAMP_BASE_API void setProcessMitigation();

XAMP_BASE_API void setCurrentThreadMitigation();

XAMP_BASE_NAMESPACE_END
