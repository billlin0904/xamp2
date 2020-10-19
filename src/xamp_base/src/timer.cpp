#include <base/platform_thread.h>
#include <base/windows_handle.h>
#include <base/logger.h>
#include <base/timer.h>

namespace xamp::base {

Timer::Timer()
	: is_stop_(true) {
}

Timer::~Timer() {
	Stop();
}

void Timer::Start(std::chrono::milliseconds timeout, TimerCallback&& callback) {
	is_stop_ = false;
	timer_.SetTimeout(timeout);
    thread_ = ThreadPool::Default().StartNew([this, timeout_routine = std::forward<TimerCallback>(callback)]() {
		SetCurrentThreadAffinity();
		SetThreadName("Timer");

		while (!is_stop_) {
			timer_.Wait();
			timeout_routine();
		}

        XAMP_LOG_DEBUG("Timer thread finished.");
	});
}

void Timer::Stop() {
    is_stop_ = true;
	if (thread_.valid()) {
		thread_.get();
	}	
}

bool Timer::IsStarted() const {
	return !is_stop_;
}

}
