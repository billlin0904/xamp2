//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <thread>

#include <base/base.h>

namespace xamp::base {

XAMP_BASE_API void SetThreadName(std::string const & name) noexcept;

XAMP_BASE_API void SetThreadAffinity(std::thread& thread, int32_t core = kDefaultAffinityCpuCore);

XAMP_BASE_API void SetCurrentThreadAffinity(int32_t core = kDefaultAffinityCpuCore);

#ifdef XAMP_OS_WIN
XAMP_BASE_API bool EnablePrivilege(std::string_view privilege, bool enable) noexcept;
XAMP_BASE_API bool ExterndProcessWorkingSetSize(size_t size) noexcept;
#endif
}
