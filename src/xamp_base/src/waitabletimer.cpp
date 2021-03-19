#include <thread>
#include <base/waitabletimer.h>
#include <base/exception.h>
#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#endif

namespace xamp::base {

#ifdef XAMP_OS_WIN
// Windows 10 2004 Sleep已經不準確, 改換為使用 CreateWaitableTimerEx
// 並加入CREATE_WAITABLE_TIMER_HIGH_RESOLUTION這個Flag.
class WaitableTimer::WaitableTimerImpl {
public:
	WaitableTimerImpl()
		: timer_(::CreateWaitableTimerEx(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS))
		, timeout_(0) {
	}

	void SetTimeout(std::chrono::milliseconds timeout) {
		timeout_ = timeout;
		Reset();
	}

	void Wait() {
		if (::WaitForSingleObject(timer_.get(), INFINITE) != WAIT_OBJECT_0) {
			throw PlatformSpecException();
		}
		Reset();
	}

	void Reset() {
		constexpr auto kMilliSecond = -10000;
		//const auto file_times = static_cast<int64_t>(-10000) * timeout_.count();
		LARGE_INTEGER timespan = { 0 };
		//timespan.LowPart = static_cast<DWORD>(file_times & 0xFFFFFFFF);
		//timespan.HighPart = static_cast<LONG>(file_times >> 32);
		timespan.QuadPart = kMilliSecond * timeout_.count();
		if (!::SetWaitableTimer(timer_.get(), &timespan, 0, nullptr, nullptr, FALSE)) {
			throw PlatformSpecException();
		}
	}

	WinHandle timer_;
	std::chrono::milliseconds timeout_;
};
#else
class WaitableTimer::WaitableTimerImpl {
public:
	WaitableTimerImpl()
		: timeout_(0)
		, tp_(std::chrono::steady_clock::now()) {		
	}

	void SetTimeout(std::chrono::milliseconds timeout) noexcept {
		timeout_ = timeout;
	}

	void Wait() noexcept {
		tp_ += timeout_;
		std::this_thread::sleep_until(tp_);
	}
	
	std::chrono::milliseconds timeout_;
	std::chrono::steady_clock::time_point tp_;
};
#endif

WaitableTimer::WaitableTimer() noexcept
	: impl_(MakeAlign<WaitableTimerImpl>()){
}

XAMP_PIMPL_IMPL(WaitableTimer)
	
void WaitableTimer::SetTimeout(std::chrono::milliseconds timeout) noexcept {
	impl_->SetTimeout(timeout);
}

void WaitableTimer::Wait() noexcept {
	impl_->Wait();
}

void MSleep(std::chrono::milliseconds timeout) {
	static thread_local WaitableTimer timer;
	timer.SetTimeout(timeout);
	timer.Wait();
}

void MSleep(int64_t timeout) {
	MSleep(std::chrono::milliseconds(timeout));
}

}
