//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>
#include <base/logger.h>
#include <base/bounded_queue.h>
#include <base/ithreadpool.h>
#include <base/windows_handle.h>

#ifdef XAMP_OS_WIN

namespace xamp::base::win32 {

class TaskScheduler;

class XAMP_BASE_API ThreadPool : public IThreadPool {
public:
	friend class TaskScheduler;

	explicit ThreadPool(const std::string_view& pool_name, uint32_t max_thread = std::thread::hardware_concurrency(), int32_t affinity = kDefaultAffinityCpuCore);

	XAMP_DISABLE_COPY(ThreadPool)

	virtual ~ThreadPool();

	void SetAffinityMask(int32_t core) override;

	void Stop() override;
private:	
	ThreadPoolHandle pool_;	
};

class XAMP_BASE_API TaskScheduler : public ITaskScheduler {
public:
	TaskScheduler(const std::string_view& pool_name, uint32_t max_thread, int32_t affinity);

	XAMP_DISABLE_COPY(TaskScheduler)

	void InitializeEnvironment(ThreadPool& threadpool);

	virtual ~TaskScheduler();

	void SubmitJob(Task&& task) override;

	void SetAffinityMask(int32_t core) override;

	void Destroy() noexcept override;

private:
	static void CALLBACK WorkCallback(PTP_CALLBACK_INSTANCE instance,
		void* context,
		PTP_WORK work);

	std::atomic<size_t> count_{ 0 };
	WorkHandle work_;
	TP_CALLBACK_ENVIRON environ_;	
	CleanupThreadGroupHandle cleanup_group_;
	std::shared_ptr<spdlog::logger> logger_;
	BoundedQueue<Task, FastMutex, FastConditionVariable> queue_;
};

}

#endif