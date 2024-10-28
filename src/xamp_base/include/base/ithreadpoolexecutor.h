//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/moveonly_function.h>
#include <base/memory.h>
#include <base/stl.h>
#include <base/platform.h>
#include <base/stop_token.h>

#include <future>
#include <type_traits>
#include <memory>

XAMP_BASE_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(ExecuteFlags, EXECUTE_NORMAL, EXECUTE_LONG_RUNNING)

class XAMP_BASE_API XAMP_NO_VTABLE ITaskScheduler {
public:
    XAMP_BASE_CLASS(ITaskScheduler)

	virtual void SubmitJob(MoveOnlyFunction&& task, ExecuteFlags flags) = 0;

    virtual size_t GetThreadSize() const = 0;

    virtual void SetBulkSize(size_t max_size) = 0;

    virtual void Destroy() noexcept = 0;
protected:
    ITaskScheduler() = default;
};

class XAMP_BASE_API XAMP_NO_VTABLE IThreadPoolExecutor {
public:
    XAMP_BASE_DISABLE_COPY_AND_MOVE(IThreadPoolExecutor)

    virtual size_t GetThreadSize() const = 0;

    virtual void SetBulkSize(size_t max_size) = 0;

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
    using ReturnType = std::invoke_result_t<F, const StopToken&, Args...>;

    // MSVC packaged_task can't be constructed from a move-only lambda
    // https://github.com/microsoft/STL/issues/321
    using PackagedTaskType = std::packaged_task<ReturnType(const StopToken&)>;

    auto task = MakeSharedPointer<PackagedTaskType>(bind_front(
        std::forward<F>(f),
        std::forward<Args>(args)...)
        );

    auto future = task->get_future();

    // note: std::unique_ptr會在SubmitJob離開lambda解構, 但是std::shared_ptr會確保lambda在解構的時候task才會解構.
    scheduler_->SubmitJob([t = std::move(task)](const auto& stop_token) {
        (*t)(stop_token);
    }, flags);

    return future;
}

struct XAMP_BASE_API ThreadPoolBuilder {
    static ScopedPtr<IThreadPoolExecutor> MakeThreadPool(const std::string_view& pool_name,
        ThreadPriority priority = ThreadPriority::PRIORITY_NORMAL,
        CpuAffinity affinity = CpuAffinity::kAll,
        uint32_t max_thread = std::thread::hardware_concurrency());

    static ScopedPtr<IThreadPoolExecutor> MakeOutputTheadPool();

    static ScopedPtr<IThreadPoolExecutor> MakeBackgroundThreadPool();

    static ScopedPtr<IThreadPoolExecutor> MakePlaybackThreadPool();
};

XAMP_BASE_NAMESPACE_END

