//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/memory.h>
#include <base/platform.h>

#include <shared_mutex>

XAMP_BASE_NAMESPACE_BEGIN

// See: https://rigtorp.se/spinlock/
class XAMP_BASE_API SpinLock final {
public:
	static constexpr int32_t kMaxSpinCount = 64;

	SpinLock() noexcept
		: flag_{ false } {
	}

	XAMP_DISABLE_COPY(SpinLock)

		void lock() noexcept {
		int32_t mask = 1;

		while (flag_.exchange(true, std::memory_order_acquire)) {
			while (flag_.load(std::memory_order_relaxed)) {
				for (int32_t i = mask; i; --i)
					CpuRelax();
				mask = mask < kMaxSpinCount ? mask << 1 : kMaxSpinCount;
			}
		}
	}

	[[nodiscard]] bool try_lock() noexcept {
		return !flag_.load(std::memory_order_relaxed)
			&& !flag_.exchange(true, std::memory_order_acquire);
	}

	void unlock() noexcept {
		flag_.store(false, std::memory_order_release);
	}
private:
	std::atomic_bool flag_;
};

using FastMutex = std::shared_mutex;

	
XAMP_BASE_NAMESPACE_END

