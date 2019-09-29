#include <base/windows_handle.h>
#include <base/waitabletimer.h>

namespace xamp::base {

class WaitableTimer::WaitableTimerImpl {
public:
	WaitableTimerImpl()
		: timer_(::CreateWaitableTimerW(nullptr, FALSE, nullptr))
		, timeout_(0) {
	}

	~WaitableTimerImpl() {
	}

	void SetTimeout(std::chrono::milliseconds timeout) {
		timeout_ = timeout;		
		Reset();
	}

	void Cancel() {
		::CancelWaitableTimer(timer_.get());
	}

	void Wait() {		
		::WaitForSingleObject(timer_.get(), INFINITE);
		Reset();
	}

private:
	void Reset() {		
		const auto file_times = static_cast<int64_t>(-10000) * timeout_.count();
		LARGE_INTEGER timespan = { 0 };
		timespan.LowPart = static_cast<DWORD>(file_times & 0xFFFFFFFF);
		timespan.HighPart = static_cast<LONG>(file_times >> 32);
		::SetWaitableTimer(timer_.get(), &timespan, 0, nullptr, nullptr, FALSE);
	}	

	WinHandle timer_;
	std::chrono::milliseconds timeout_;
};

WaitableTimer::WaitableTimer()
	: impl_(MakeAlign<WaitableTimerImpl>()) {
}

XAMP_PIMPL_IMPL(WaitableTimer)

void WaitableTimer::SetTimeout(std::chrono::milliseconds timeout) {
	impl_->SetTimeout(timeout);
}

void WaitableTimer::Cancel() {
	impl_->Cancel();
}

void WaitableTimer::Wait() {
	impl_->Wait();
}

}
