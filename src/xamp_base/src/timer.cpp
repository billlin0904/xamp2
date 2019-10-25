#include <base/windows_handle.h>
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
    thread_ = std::async(std::launch::async, [this, timeout_routine = std::forward<TimerCallback>(callback)]() {
		while (!is_stop_) {
			timer_.Wait();
			timeout_routine();
		}
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
