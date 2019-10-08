#pragma once

#include <chrono>

#include <base/base.h>
#include <base/align_ptr.h>

namespace xamp::base {

class XAMP_BASE_API WaitableTimer final {
public:
	WaitableTimer();

	XAMP_PIMPL(WaitableTimer)

	void SetTimeout(std::chrono::milliseconds timeout);

	void Cancel();

	void Wait();
private:
	class WaitableTimerImpl;
	AlignPtr<WaitableTimerImpl> impl_;
};

}
