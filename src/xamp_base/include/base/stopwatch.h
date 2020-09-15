//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <base/base.h>

namespace xamp::base {

class XAMP_BASE_API Stopwatch final {
public:
	Stopwatch() noexcept;
	
	void Reset() noexcept;

	template <typename Resolution = std::chrono::microseconds>
	Resolution Elapsed() noexcept {
		return std::chrono::duration_cast<Resolution>(clock_.now() - start_time_);
	}

private:
	std::chrono::high_resolution_clock clock_;
	std::chrono::high_resolution_clock::time_point start_time_;
};

}

