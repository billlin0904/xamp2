//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <atomic>
#include <thread>

#include <base/platform.h>
#include <base/base.h>
#include <base/logger.h>
#include <base/waitabletimer.h>

namespace xamp::base {

class XAMP_BASE_API Timer final {
public:
	Timer();

	~Timer();

	XAMP_DISABLE_COPY(Timer)

	template <typename TimerCallback>
	void Start(std::chrono::milliseconds timeout, TimerCallback&& callback) {
		if (!is_stop_) {
			return;
		}

		is_stop_ = false;
		thread_ = std::thread([this, timeout, callback]() {
			SetThreadName("Timer");
			WaitableTimer timer;
			XAMP_LOG_DEBUG("Timer thread running!");

			while (!is_stop_) {				
				callback();
				timer.SetTimeout(timeout);
				timer.Wait();
			}

			XAMP_LOG_DEBUG("Timer thread finished.");
		});
	}

	void Stop();

	bool IsStarted() const;

private:
	std::atomic<bool> is_stop_;
	std::thread thread_;
};

}
