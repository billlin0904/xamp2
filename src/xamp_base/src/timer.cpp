#include <base/logger.h>
#include <base/timer.h>

namespace xamp::base {

Timer::Timer()
	: is_stop_(true) {
}

Timer::~Timer() {
	Stop();
}

void Timer::Stop() {
    is_stop_ = true;
	if (thread_.joinable()) {
		thread_.join();
	}	
}

bool Timer::IsStarted() const {
	return !is_stop_;
}

}
