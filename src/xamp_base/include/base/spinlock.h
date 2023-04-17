//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>
#include <base/platform.h>

XAMP_BASE_NAMESPACE_BEGIN

class Spinlock {
public:
	XAMP_DISABLE_COPY(Spinlock)

	Spinlock() noexcept
		: flag_(0) {
	}

	void lock() noexcept {
		constexpr auto kMaxSpinCount = 8;

		for (auto i = 0; flag_.load(std::memory_order_relaxed) 
			|| flag_.exchange(1, std::memory_order_acquire); ++i) {
			if (i == kMaxSpinCount) {
				i = 0;
				CpuRelax();
			}
		}
	}

	bool try_lock() noexcept {
		return !flag_.load(std::memory_order_relaxed)
			|| !flag_.exchange(1, std::memory_order_acquire);
	}

	void unlock() noexcept {
		flag_.store(0, std::memory_order_release);
	}
private:
	std::atomic<uint32_t> flag_;
};

XAMP_BASE_NAMESPACE_END

