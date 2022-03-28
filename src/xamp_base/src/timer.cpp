#include <thread>
#include <base/windows_handle.h>
#include <base/exception.h>
#include <base/platform.h>
#include <base/waitabletimer.h>
#include <base/timer.h>

namespace xamp::base {

#if defined(XAMP_OS_WIN)

class TimerQueueTimer {
public:
	TimerQueueTimer()
		: timer_queue_(nullptr)
		, timer_(nullptr) {		
	}

	void Reset(HANDLE timer_queue, HANDLE timer) {
		Close();
		timer_queue_ = timer_queue;
		timer_ = timer;
	}

	void Close() {
		if (!timer_queue_ || !timer_) {
			return;
		}
		::DeleteTimerQueueTimer(timer_queue_,
			timer_,
			INVALID_HANDLE_VALUE);
		timer_queue_ = nullptr;
		timer_ = nullptr;
	}

	~TimerQueueTimer() {
		Close();
	}

	XAMP_DISABLE_COPY(TimerQueueTimer)
private:
	HANDLE timer_queue_;
	HANDLE timer_;
};

class Timer::TimerImpl {
public:
	TimerImpl() = default;

	~TimerImpl() {
		Stop();
	}

	void Start(std::chrono::milliseconds interval, std::function<void()> callback) {
		if (!is_stop_) {
			return;
		}

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
			WT_EXECUTEINTIMERTHREAD | WT_EXECUTELONGFUNCTION)) {
			throw PlatformSpecException();
		}
		timer_.Reset(timer_queue_.get(), timer);
		callback_ = std::move(callback);
		is_stop_ = false;
	}

	void Stop() {
		if (is_stop_) {
			return;
		}
		is_stop_ = true;
		timer_.Close();
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
	TimerQueueTimer timer_;
	std::function<void()> callback_;
};
#else
class Timer::TimerImpl {
public:
	TimerImpl() = default;

	~TimerImpl() {
		Stop();
	}

	void Start(std::chrono::milliseconds interval, std::function<void()> callback) {
		if (!is_stop_) {
			return;
		}

		is_stop_ = false;
        thread_ = std::thread([this, interval, callback]() {
			SetThreadName("Timer");
			WaitableTimer timer;
			while (!is_stop_) {
				callback();
                timer.SetTimeout(interval);
				timer.Wait();
			}
		});
	}

	bool IsStarted() const {
		return !is_stop_;
	}

	void Stop() {
		is_stop_ = true;
		if (thread_.joinable()) {
			thread_.join();
		}
	}
private:
	std::atomic<bool> is_stop_{ true };
	std::thread thread_;
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
