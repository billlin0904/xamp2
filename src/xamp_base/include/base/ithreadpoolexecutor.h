//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/moveonly_function.h>
#include <base/memory.h>
#include <base/stl.h>
#include <base/platform.h>

#include <stop_token>
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
    // When Spawn receives f, it effectively only captures a "reference" to the lambda expression,
    // rather than the lambda’s value itself. Therefore, std::forward<Func>(f) will not move f,
    // because f is an lvalue here, and std::forward does not turn an lvalue into an rvalue.
    //static_assert(std::is_lvalue_reference_v<F>, "Func must be l value reference.");

    using ReturnType = std::invoke_result_t<F, const std::stop_token&, Args...>;

    // MSVC packaged_task can't be constructed from a move-only lambda
    // https://github.com/microsoft/STL/issues/321
    using PackagedTaskType = std::packaged_task<ReturnType(const std::stop_token&)>;

    auto task = std::make_shared<PackagedTaskType>(bind_front(
        std::forward<F>(f),
        std::forward<Args>(args)...)
        );

    auto future = task->get_future();

    // std::unique_ptr will destruct task when the lambda in SubmitJob finishes,
    // but std::shared_ptr ensures that task will only be destructed when the lambda itself is fully released.
    scheduler_->SubmitJob([t = std::move(task)](const auto& stop_token) {
        (*t)(stop_token);
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

