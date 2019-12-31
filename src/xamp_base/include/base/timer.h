//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <functional>
#include <atomic>
#include <future>

#include <base/base.h>
#include <base/memory.h>
#include <base/waitabletimer.h>

namespace xamp::base {

using TimerCallback = std::function<void()>;

class XAMP_BASE_API Timer final {
public:
	Timer();

	~Timer();

	XAMP_DISABLE_COPY(Timer)

	void Start(std::chrono::milliseconds timeout, TimerCallback&& callback);

	void Stop();

	bool IsStarted() const;

private:
	std::atomic<bool> is_stop_;
    std::future<void> thread_;
	WaitableTimer timer_;
};

}
