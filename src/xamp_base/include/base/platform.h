//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <thread>

#include <base/enum.h>
#include <base/base.h>

namespace xamp::base {

MAKE_XAMP_ENUM(
    ThreadPriority,
    BACKGROUND,
    NORMAL,
    HIGHEST)

MAKE_XAMP_ENUM(
    TaskSchedulerPolicy,
    ROUND_ROBIN_POLICY,
    RANDOM_POLICY,
    LEAST_LOAD_POLICY)

XAMP_BASE_API void SetThreadPriority(ThreadPriority priority) noexcept;

XAMP_BASE_API void SetThreadName(std::string const & name) noexcept;

XAMP_BASE_API void SetThreadAffinity(std::thread& thread, int32_t core = kDefaultAffinityCpuCore) noexcept;

XAMP_BASE_API std::string GetCurrentThreadId();

XAMP_BASE_API std::string MakeTempFileName();

XAMP_BASE_API std::string MakeUuidString();

XAMP_BASE_API bool IsDebuging();

XAMP_BASE_API bool VirtualMemoryLock(void* address, size_t size);

XAMP_BASE_API bool VirtualMemoryUnLock(void* address, size_t size);

XAMP_BASE_API void MSleep(std::chrono::milliseconds timeout);

#ifdef XAMP_OS_WIN
XAMP_BASE_API bool ExtendProcessWorkingSetSize(size_t size) noexcept;
XAMP_BASE_API bool SetProcessWorkingSetSize(size_t working_set_size) noexcept;
#endif
}
