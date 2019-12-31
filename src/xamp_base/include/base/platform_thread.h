//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/base.h>

namespace xamp::base {

class PlatformThread {
public:
	PlatformThread() = delete;

	XAMP_DISABLE_COPY(PlatformThread);

    static XAMP_BASE_API void SetThreadName(const std::string& name);
};

}
