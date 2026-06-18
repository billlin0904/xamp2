//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

class XAMP_BASE_API Stopwatch final {
public:
	Stopwatch() ;
	
	void reset() ;

	template <typename Resolution = std::chrono::microseconds>
	[[nodiscard]] Resolution elapsed() const {
		return std::chrono::duration_cast<Resolution>(Clock::now() - start_time_);
	}

	[[nodiscard]] double elapsedSeconds() const {		
		return static_cast<double>(elapsed<std::chrono::milliseconds>().count()) / 1000.0;
	}

private:
	using Clock = std::chrono::steady_clock;
	Clock clock_;
	Clock::time_point start_time_;
};

XAMP_BASE_NAMESPACE_END
