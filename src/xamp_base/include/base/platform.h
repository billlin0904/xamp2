//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <thread>

#include <base/base.h>

namespace xamp::base {

XAMP_BASE_API void SetThreadName(std::string const & name) noexcept;

XAMP_BASE_API void SetThreadAffinity(std::thread& thread, int32_t core = kDefaultAffinityCpuCore) noexcept;

XAMP_BASE_API std::string GetCurrentThreadId();

XAMP_BASE_API std::string MakeTempFileName();

XAMP_BASE_API std::string MakeUuidString();

XAMP_BASE_API bool IsDebuging();

#ifdef XAMP_OS_WIN
XAMP_BASE_API bool ExtendProcessWorkingSetSize(size_t size) noexcept;
XAMP_BASE_API bool InitWorkingSetSize(size_t working_set_size) noexcept;
#endif
}
