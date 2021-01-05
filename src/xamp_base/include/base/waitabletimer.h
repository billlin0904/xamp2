//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <thread>

#include <base/base.h>

namespace xamp::base {

class XAMP_BASE_API WaitableTimer final {
public:
	WaitableTimer() noexcept;

	~WaitableTimer() noexcept = default;

	void SetTimeout(std::chrono::milliseconds timeout) noexcept;

	void Wait() noexcept;
private:
	std::chrono::milliseconds timeout_;
	std::chrono::steady_clock::time_point tp_;
};

}
