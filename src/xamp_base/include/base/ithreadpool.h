//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <future>
#include <vector>
#include <type_traits>
#include <memory>

#include <base/base.h>
#include <base/task.h>
#include <base/align_ptr.h>
#include <base/stl.h>
#include <base/platform.h>

namespace xamp::base {

inline constexpr uint32_t kMaxThread = 32;

template <typename T>
using SharedFuture = std::shared_future<T>;

class XAMP_BASE_API XAMP_NO_VTABLE ITaskScheduler {
public:
    XAMP_BASE_CLASS(ITaskScheduler)

	virtual void SubmitJob(Task&& task) = 0;

    virtual uint32_t GetThreadSize() const = 0;

    virtual void Destroy() noexcept = 0;
protected:
    ITaskScheduler() = default;
};

// note: 輕量化與並且快速反應的Playback thread.
class XAMP_BASE_API XAMP_NO_VTABLE IThreadPool {
public:
    XAMP_BASE_DISABLE_COPY_AND_MOVE(IThreadPool)

    virtual uint32_t GetThreadSize() const = 0;

    virtual void Stop() = 0;

    template <typename F, typename... Args>
    decltype(auto) Spawn(F&& f, Args&&... args);

protected:
    explicit IThreadPool(AlignPtr<ITaskScheduler> scheduler)
	    : scheduler_(std::move(scheduler)) {
    }

    AlignPtr<ITaskScheduler> scheduler_;
};

template <typename F, typename ... Args>
decltype(auto) IThreadPool::Spawn(F&& f, Args&&... args) {
    using ReturnType = std::invoke_result_t<F, size_t, Args...>;

    // MSVC packaged_task can't be constructed from a move-only lambda
    // https://github.com/microsoft/STL/issues/321
    using PackagedTaskType = std::packaged_task<ReturnType(size_t)>;

    auto task = MakeAlignedShared<PackagedTaskType>(bind_front(
        std::forward<F>(f),
        std::forward<Args>(args)...)
        );

    auto future = task->get_future();

    // note: unique_ptr會在SubmitJob離開lambda解構, 但是shared_ptr會確保lambda在解構的時候task才會解構.
    scheduler_->SubmitJob([t = std::move(task)](size_t thread_index) {
        (*t)(thread_index);
    });

    return future.share();
}

XAMP_BASE_API AlignPtr<IThreadPool> MakeThreadPool(
    const std::string_view& pool_name,
    ThreadPriority priority = ThreadPriority::NORMAL,
    uint32_t max_thread = std::thread::hardware_concurrency(),
    CpuAffinity affinity = kDefaultAffinityCpuCore,
    TaskSchedulerPolicy policy = TaskSchedulerPolicy::RANDOM_POLICY,
    TaskStealPolicy steal_policy = TaskStealPolicy::CONTINUATION_STEALING_POLICY);

XAMP_BASE_API IThreadPool& GetPlaybackThreadPool();

XAMP_BASE_API IThreadPool& GetWASAPIThreadPool();

}
