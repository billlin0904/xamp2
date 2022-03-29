#include <base/win32/win32_threadpool.h>

#ifdef XAMP_OS_WIN
namespace xamp::base::win32 {

TaskScheduler::TaskScheduler(const std::string_view& pool_name)
	: queue_(4096) {
	logger_ = Logger::GetInstance().GetLogger(pool_name.data());
}

void TaskScheduler::InitializeEnvironment(ThreadPool& threadpool) {
	::InitializeThreadpoolEnvironment(&environ_);
	::SetThreadpoolCallbackPool(&environ_, threadpool.pool_.get());
	cleanup_group_.reset(::CreateThreadpoolCleanupGroup());
	::SetThreadpoolCallbackCleanupGroup(&environ_, cleanup_group_.get(), nullptr);
	work_.reset(::CreateThreadpoolWork(
		WorkCallback,
		this,
		&environ_));	
}

TaskScheduler::~TaskScheduler() {
	Destroy();	
}

uint32_t TaskScheduler::GetThreadSize() const {
	return 0;
}

void TaskScheduler::Destroy() noexcept {
	if (!work_.is_valid()) {
		return;
	}
	constexpr BOOL cancel_pending = TRUE;
	::WaitForThreadpoolWorkCallbacks(work_.get(), cancel_pending);
	work_.reset();
	queue_.WakeupForShutdown();
	::CloseThreadpoolCleanupGroupMembers(cleanup_group_.get(), TRUE, nullptr);	
	cleanup_group_.reset();
	::DestroyThreadpoolEnvironment(&environ_);		
}

void CALLBACK TaskScheduler::WorkCallback(PTP_CALLBACK_INSTANCE instance,
	void* context,
	PTP_WORK work) {
	auto* scheduler = static_cast<TaskScheduler*>(context);
	Task task;
	if (scheduler->queue_.Dequeue(task)) {
		const auto thread_id = ::GetCurrentThreadId();
		task(thread_id);
	}
}

void TaskScheduler::SubmitJob(Task&& task) {
	queue_.Enqueue(task);
	::SubmitThreadpoolWork(work_.get());
}

ThreadPool::ThreadPool(const std::string_view& pool_name, uint32_t max_thread)
	: IThreadPool(MakeAlign<ITaskScheduler, TaskScheduler>(pool_name))
	, pool_(::CreateThreadpool(nullptr)) {
	dynamic_cast<TaskScheduler*>(scheduler_.get())->InitializeEnvironment(*this);
	::SetThreadpoolThreadMinimum(pool_.get(), 1);
	::SetThreadpoolThreadMaximum(pool_.get(), max_thread);
}

ThreadPool::~ThreadPool() {
	Stop();
}

void ThreadPool::Stop() {
	scheduler_->Destroy();
	pool_.reset();
}

uint32_t ThreadPool::GetThreadSize() const {
	return 0;
}

}
#endif
