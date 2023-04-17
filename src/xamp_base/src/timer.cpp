#include <base/timer.h>

#include <base/platfrom_handle.h>
#include <base/exception.h>
#include <base/waitabletimer.h>

#if defined(XAMP_OS_MAC)
#include <dispatch/dispatch.h>
#endif

XAMP_BASE_NAMESPACE_BEGIN

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
        callback_ = callback;

        timer_queue_ = ::dispatch_queue_create("org.xamp2.timerqueue", nullptr);
        timer_ = ::dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, timer_queue_);

        using namespace std::chrono;
        auto ineterval_nano_sec = duration_cast<nanoseconds>(interval);

        ::dispatch_source_set_timer(timer_, dispatch_walltime(nullptr, 0), ineterval_nano_sec.count(), 0);
        ::dispatch_set_context(timer_, this);
        ::dispatch_source_set_event_handler_f(timer_, TimerCallback);
        ::dispatch_resume(timer_);
	}

	bool IsStarted() const {
		return !is_stop_;
	}

	void Stop() {
        if (is_stop_) {
            return;
        }
		is_stop_ = true;
        ::dispatch_source_cancel(timer_);
        ::dispatch_release(timer_);
        ::dispatch_release(timer_queue_);
        timer_ = nullptr;
	}
private:
    static void TimerCallback(void *arg) {
        const auto* timer = static_cast<TimerImpl*>(arg);
        try {
            timer->callback_();
        } catch (...) {
        }
    }

    std::atomic<bool> is_stop_{true};
    dispatch_queue_t timer_queue_{nullptr};
    dispatch_source_t timer_{nullptr};
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

XAMP_BASE_NAMESPACE_END