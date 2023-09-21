//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

class XAMP_BASE_API Stopwatch final {
public:
	Stopwatch() noexcept;
	
	void Reset() noexcept;

	template <typename Resolution = std::chrono::microseconds>
	[[nodiscard]] Resolution Elapsed() const noexcept {
		return std::chrono::duration_cast<Resolution>(Clock::now() - start_time_);
	}

	[[nodiscard]] double ElapsedSeconds() const noexcept {		
		return static_cast<double>(Elapsed<std::chrono::milliseconds>().count()) / 1000.0;
	}

private:
	using Clock = std::chrono::steady_clock;
	Clock clock_;
	Clock::time_point start_time_;
};

XAMP_BASE_NAMESPACE_END
