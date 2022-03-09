#include <base/windows_handle.h>
#include <base/exception.h>
#include <base/timer.h>

namespace xamp::base {

#if defined(XAMP_OS_WIN)

class Timer::TimerImpl {
public:
	TimerImpl() = default;

	void Start(std::chrono::milliseconds interval, std::function<void()> callback) {
		if (!is_stop_) {
			return;
		}

		is_stop_ = false;

		constexpr bool immediately = false;
		constexpr bool once = false;

		timer_queue_.reset(::CreateTimerQueue());

		HANDLE timer = nullptr;
		if (!::CreateTimerQueueTimer(&timer,
			timer_queue_.get(),
			TimerProc,
			this,
			immediately ? 0 : interval.count(),
			once ? 0 : interval.count(),
			WT_EXECUTEINTIMERTHREAD)) {
			throw PlatformSpecException();
		}
		timer_ = timer;
		callback_ = std::move(callback);
	}

	void Stop() {
		if (is_stop_) {
			return;
		}
		is_stop_ = true;
		::DeleteTimerQueueTimer(timer_queue_.get(), timer_, INVALID_HANDLE_VALUE);
		timer_queue_.reset();
		callback_ = nullptr;
	}

	bool IsStarted() const {
		return !is_stop_;
	}
private:
	static void CALLBACK TimerProc(void* param, BOOLEAN timer_called) {
		const auto* timer = static_cast<TimerImpl*>(param);
		try {
			timer->callback_();
		} catch (...) {
		}
	}

	std::atomic<bool> is_stop_{true};
	TimerQueueHandle timer_queue_;
	HANDLE timer_;
	std::function<void()> callback_;
};

#endif

XAMP_PIMPL_IMPL(Timer)

Timer::Timer()
	: impl_(MakeAlign<TimerImpl>()) {
}

void Timer::Start(std::chrono::milliseconds interval, std::function<void()> callback) {
	impl_->Start(interval, std::move(callback));
}

void Timer::Stop() {
	impl_->Stop();
}

bool Timer::IsStarted() const {
	return impl_->IsStarted();
}

}
