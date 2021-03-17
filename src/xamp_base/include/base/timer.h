//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <atomic>
#include <future>

#include <base/platform_thread.h>
#include <base/base.h>
#include <base/logger.h>
#include <base/threadpool.h>
#include <base/waitabletimer.h>

namespace xamp::base {

class XAMP_BASE_API Timer final {
public:
	Timer();

	~Timer();

	XAMP_DISABLE_COPY(Timer)

	template <typename TimerCallback>
	void Start(std::chrono::milliseconds timeout, TimerCallback&& callback) {
		is_stop_ = false;
		thread_ = ThreadPool::GetInstance().Run([this, timeout, callback]() {
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
	std::shared_future<void> thread_;
};

}
