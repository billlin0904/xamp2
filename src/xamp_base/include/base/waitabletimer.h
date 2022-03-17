//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <base/base.h>
#include <base/memory.h>
#include <base/align_ptr.h>

namespace xamp::base {

class XAMP_BASE_API WaitableTimer final {
public:
	WaitableTimer() noexcept;

	XAMP_PIMPL(WaitableTimer)

	void SetTimeout(std::chrono::milliseconds timeout) noexcept;

	void Wait();
private:
	class WaitableTimerImpl;
	AlignPtr<WaitableTimerImpl> impl_;
};

}
