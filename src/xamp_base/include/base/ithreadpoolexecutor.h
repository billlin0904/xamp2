//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/task.h>
#include <base/memory.h>
#include <base/stl.h>
#include <base/platform.h>

#include <stop_token>
#include <future>
#include <type_traits>
#include <memory>

XAMP_BASE_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(ExecuteFlags, 
    EXECUTE_NORMAL, 
    EXECUTE_LONG_RUNNING)

class XAMP_BASE_API XAMP_NO_VTABLE ITaskScheduler {
public:
    XAMP_BASE_CLASS(ITaskScheduler)

	virtual void SubmitJob(Task task, ExecuteFlags flags) = 0;

    virtual size_t GetThreadSize() const = 0;

    virtual void Destroy() noexcept = 0;
protected:
    ITaskScheduler() = default;
};

class XAMP_BASE_API XAMP_NO_VTABLE IThreadPoolExecutor {
public:
    XAMP_BASE_DISABLE_COPY_AND_MOVE(IThreadPoolExecutor)

    virtual size_t GetThreadSize() const = 0;

    virtual void Stop() = 0;

    template <typename F, typename... Args>
    decltype(auto) Spawn(F&& f, Args&&... args, ExecuteFlags flags = ExecuteFlags::EXECUTE_NORMAL);

protected:
    explicit IThreadPoolExecutor(ScopedPtr<ITaskScheduler> scheduler)
	    : scheduler_(std::move(scheduler)) {
    }

    ScopedPtr<ITaskScheduler> scheduler_;
};

template <typename F, typename ... Args>
decltype(auto) IThreadPoolExecutor::Spawn(F&& f, Args&&... args, ExecuteFlags flags) {
    using ReturnType = std::invoke_result_t<F, const std::stop_token&, Args...>;

    using PackagedTaskType = std::packaged_task<ReturnType(const std::stop_token&)>;

    PackagedTaskType task(
        std::forward<F>(f),
        std::forward<Args>(args)...);

    auto future = task.get_future();

    scheduler_->SubmitJob([t = std::move(task)](const auto& stop_token) mutable {
        t(stop_token);
    }, flags);

    return future;
}

struct XAMP_BASE_API ThreadPoolBuilder {
    static std::shared_ptr<IThreadPoolExecutor> MakeThreadPool(const std::string_view& pool_name,
        uint32_t max_thread = std::thread::hardware_concurrency(),
        size_t bulk_size = std::thread::hardware_concurrency() / 2,
        ThreadPriority priority = ThreadPriority::PRIORITY_NORMAL);

    static std::shared_ptr<IThreadPoolExecutor> MakeBackgroundThreadPool();

    static std::shared_ptr<IThreadPoolExecutor> MakePlaybackThreadPool();

	static std::shared_ptr<IThreadPoolExecutor> MakePlayerThreadPool();
};

XAMP_BASE_NAMESPACE_END

