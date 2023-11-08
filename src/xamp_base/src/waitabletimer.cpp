#include <base/waitabletimer.h>

#include <base/exception.h>
#include <base/logger.h>
#include <base/platfrom_handle.h>

#ifdef XAMP_OS_WIN
#include <timeapi.h>
#endif

#include <thread>

XAMP_BASE_NAMESPACE_BEGIN

inline constexpr uint32_t kDesiredSchedulerMS = 1;

class XAMP_NO_VTABLE IWaitableTimer {
public:
	virtual ~IWaitableTimer() = default;
	virtual void SetTimeout(std::chrono::milliseconds timeout) noexcept = 0;
	virtual void Wait() = 0;
protected:
	IWaitableTimer() = default;
};

#ifdef XAMP_OS_WIN
class TimePeriod {
public:
	TimePeriod() noexcept {
		TIMECAPS tc{ 0 };
		if (::timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR) {
			const auto timer_res = (std::min)((std::max)(tc.wPeriodMin, kDesiredSchedulerMS), tc.wPeriodMax);
			sleep_is_granular_ = (::timeBeginPeriod(timer_res) == TIMERR_NOERROR);
		} else {
			sleep_is_granular_ = false;
		}
	}

	~TimePeriod() noexcept {
		if (sleep_is_granular_) {
			::timeEndPeriod(kDesiredSchedulerMS);
		}
	}

	bool IsSleepSranular() const noexcept {
		return sleep_is_granular_;
	}
private:
	bool sleep_is_granular_;
};

// Windows 10 2004 Sleep已經不準確, 改換為使用 CreateWaitableTimerEx
// 並加入CREATE_WAITABLE_TIMER_HIGH_RESOLUTION這個Flag.
// 建議是在全套ACP使用, 所以目前不使用APC Timer方式.
class APCWaitableTimerImpl : public IWaitableTimer {
public:
	APCWaitableTimerImpl()
		: timer_(::CreateWaitableTimerEx(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS))
		, timeout_(0) {
	}

	void SetTimeout(std::chrono::milliseconds timeout) noexcept override {
		timeout_ = timeout;
		Reset();
	}

	void Wait() override {
		if (::WaitForSingleObject(timer_.get(), INFINITE) != WAIT_OBJECT_0) {
			throw PlatformException();
		}
		Reset();
	}

	void Reset() {
		constexpr auto kMilliSecond = -10000;
		LARGE_INTEGER timespan = { 0 };
		timespan.QuadPart = kMilliSecond * timeout_.count();
		if (!::SetWaitableTimer(timer_.get(), &timespan, 0, nullptr, nullptr, FALSE)) {
			throw PlatformException();
		}
	}

	WinHandle timer_;
	std::chrono::milliseconds timeout_;
};
#endif

class StdWaitableTimerImpl : public IWaitableTimer {
public:
	StdWaitableTimerImpl()
		: timeout_(0)
		, tp_(std::chrono::steady_clock::now()) {
	}

	void SetTimeout(std::chrono::milliseconds timeout) noexcept override {
		timeout_ = timeout;
	}

	void Wait() override {
		tp_ += timeout_;
		std::this_thread::sleep_until(tp_);
	}

	std::chrono::milliseconds timeout_;
	std::chrono::steady_clock::time_point tp_;
};

class WaitableTimer::WaitableTimerImpl {
public:
	WaitableTimerImpl()
#ifdef XAMP_OS_WIN
		: impl_(time_period_.IsSleepSranular() 
			? MakeAlign<IWaitableTimer, StdWaitableTimerImpl>()
			: MakeAlign<IWaitableTimer, APCWaitableTimerImpl>()) {
#else
		: impl_(MakeAlign<IWaitableTimer, StdWaitableTimerImpl>()) {
#endif
	}

	void SetTimeout(std::chrono::milliseconds timeout) noexcept {
		impl_->SetTimeout(timeout);
	}

	void Wait() {
		impl_->Wait();
	}
#ifdef XAMP_OS_WIN
	static TimePeriod time_period_;
#endif
	AlignPtr<IWaitableTimer> impl_;
};
#ifdef XAMP_OS_WIN
TimePeriod WaitableTimer::WaitableTimerImpl::time_period_;
#endif
WaitableTimer::WaitableTimer() noexcept
	: impl_(MakeAlign<WaitableTimerImpl>()) {
}

XAMP_PIMPL_IMPL(WaitableTimer)
	
void WaitableTimer::SetTimeout(std::chrono::milliseconds timeout) noexcept {
	impl_->SetTimeout(timeout);
}

void WaitableTimer::Wait() {
	impl_->Wait();
}

XAMP_BASE_NAMESPACE_END
