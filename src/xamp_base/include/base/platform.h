//====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <thread>
#include <vector>

#include <base/enum.h>
#include <base/base.h>
#include <base/jthread.h>

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
        0; // 在 macOS 上，超時為 0 表示無限等待
    #endif

XAMP_BASE_API void SetThreadPriority(JThread& thread, ThreadPriority priority);

XAMP_BASE_API void SetThreadName(std::wstring const & name);

XAMP_BASE_API std::string GetCurrentThreadId();

XAMP_BASE_API bool IsDebuging();

XAMP_BASE_API bool VirtualMemoryLock(void* address, size_t size);

XAMP_BASE_API bool VirtualMemoryUnLock(void* address, size_t size);

XAMP_BASE_API void MSleep(std::chrono::milliseconds timeout);
/*
* Futex wait implementation.
*
* @param[out] to_wait_on The atomic variable to wait on.
* @param[in] expected The expected value of the atomic variable.
* @param[in] to The time to wait for.
*
* @return 0 if the atomic variable was woken up, -1 if the wait timed out.
*/
XAMP_BASE_API int32_t AtomicWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, const timespec* to) noexcept;

XAMP_BASE_API bool AtomicWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, uint32_t milliseconds) noexcept;

XAMP_BASE_API void AtomicWakeSingle(std::atomic<uint32_t>& to_wake) noexcept;

XAMP_BASE_API void AtomicWakeAll(std::atomic<uint32_t>& to_wake) noexcept;

XAMP_BASE_API uint64_t GenRandomSeed() noexcept;

XAMP_BASE_API uint64_t GetSystemEntropy() noexcept;

XAMP_BASE_API void CpuRelax() noexcept;

XAMP_BASE_API void Assert(const char* message, const char* file, uint32_t line);

XAMP_BASE_API std::string GetSequentialUUID();

#ifdef XAMP_OS_WIN
XAMP_BASE_API void SetCurrentProcessPriority(ProcessPriority priority);

XAMP_BASE_API void SetProcessPriority(int32_t pid, ProcessPriority priority);
XAMP_BASE_API bool EnablePrivilege(std::string_view privilege, bool enable);
XAMP_BASE_API bool ExtendProcessWorkingSetSize(size_t size);
XAMP_BASE_API bool SetProcessWorkingSetSize(size_t working_set_size);
XAMP_BASE_API void SetProcessMitigation();
XAMP_BASE_API void SetThreadMitigation();
#endif

XAMP_BASE_NAMESPACE_END
