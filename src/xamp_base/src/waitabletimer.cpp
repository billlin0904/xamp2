#include <base/waitabletimer.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#else
#include <thread>
#include <base/posix_handle.h>
#endif

namespace xamp::base {

#ifdef XAMP_OS_WIN
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
#else
class WaitableTimer::WaitableTimerImpl {
public:
    WaitableTimerImpl()
        : timeout_(0) {
    }

    ~WaitableTimerImpl() {
        Cancel();
    }

    void SetTimeout(std::chrono::milliseconds timeout) {
        timeout_ = timeout;
    }

    void Cancel() {
        cond_.notify_all();
    }

    void Wait() {
        //std::unique_lock<std::mutex> guard{mutex_};
        //cond_.wait_for(guard, timeout_);
        std::this_thread::sleep_for(timeout_);
    }

private:
    std::chrono::milliseconds timeout_;
    std::condition_variable cond_;
    std::mutex mutex_;
};
#endif

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
