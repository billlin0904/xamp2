#pragma once

#include <chrono>

#include <base/base.h>
#include <base/memory.h>

namespace xamp::base {

class XAMP_BASE_API WaitableTimer {
public:
	WaitableTimer();

	XAMP_PIMPL(WaitableTimer)

	void SetTimeout(std::chrono::milliseconds timeout);

	void Cancel();

	void Wait();
private:
	class WaitableTimerImpl;
	std::unique_ptr<WaitableTimerImpl> impl_;
};

}
