//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>
#include <base/logger.h>
#include <base/blocking_queue.h>
#include <base/ithreadpool.h>
#include <base/windows_handle.h>

#ifdef XAMP_OS_WIN

namespace xamp::base::win32 {

class TaskScheduler;

class XAMP_BASE_API ThreadPool : public IThreadPool {
public:
	friend class TaskScheduler;

	explicit ThreadPool(const std::string_view& pool_name, 
		uint32_t max_thread = std::thread::hardware_concurrency());

	XAMP_DISABLE_COPY(ThreadPool)

	~ThreadPool() override;

	void Stop() override;

	uint32_t GetThreadSize() const override;
private:	
	ThreadPoolHandle pool_;	
};

class XAMP_BASE_API TaskScheduler : public ITaskScheduler {
public:
	explicit TaskScheduler(const std::string_view& pool_name);

	XAMP_DISABLE_COPY(TaskScheduler)

	void InitializeEnvironment(ThreadPool& threadpool);

	~TaskScheduler() override;

	void SubmitJob(Task&& task) override;

	void Destroy() noexcept override;

	uint32_t GetThreadSize() const override;

private:
	static void CALLBACK WorkCallback(PTP_CALLBACK_INSTANCE instance,
		void* context,
		PTP_WORK work);

	WorkHandle work_;
	TP_CALLBACK_ENVIRON environ_;	
	CleanupThreadGroupHandle cleanup_group_;
	BlockingQueue<Task> queue_;
	std::shared_ptr<spdlog::logger> logger_;
};

}

#endif