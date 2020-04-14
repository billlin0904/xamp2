#pragma once

#include <chrono>
#include <thread>

#include <base/base.h>

namespace xamp::base {

class XAMP_BASE_API WaitableTimer final {
public:
	WaitableTimer() noexcept;

	~WaitableTimer() noexcept = default;

	void SetTimeout(std::chrono::milliseconds timeout) noexcept {
		timeout_ = timeout;
	}

	void Wait() noexcept {
		tp_ += timeout_;
		std::this_thread::sleep_until(tp_);
	}
private:
	std::chrono::milliseconds timeout_;
	std::chrono::steady_clock::time_point tp_;
};

}
