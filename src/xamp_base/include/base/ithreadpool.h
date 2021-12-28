//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <future>
#include <atomic>
#include <vector>
#include <optional>
#include <type_traits>
#include <memory>

#include <base/base.h>
#include <base/stopwatch.h>
#include <base/align_ptr.h>
#include <base/stl.h>
#include <base/bounded_queue.h>

#include <base/logger.h>

namespace xamp::base {

inline constexpr uint32_t kMaxThread = 32;

class TaskWrapper final {
public:
    template <typename Func>
    TaskWrapper(Func&& f)
        : impl_(MakeAlign<ImplBase, ImplType<Func>>(std::forward<Func>(f))) {
    }

    XAMP_ALWAYS_INLINE void operator()(const size_t thread_index) const {
	    impl_->Call(thread_index);
    }

    TaskWrapper() = default;
	
    TaskWrapper(TaskWrapper&& other) noexcept
		: impl_(std::move(other.impl_)) {	    
    }
	
    TaskWrapper& operator=(TaskWrapper&& other) noexcept {
        impl_ = std::move(other.impl_);
        return *this;
    }
	
    XAMP_DISABLE_COPY(TaskWrapper)
	
private:
    struct XAMP_NO_VTABLE ImplBase {
        virtual ~ImplBase() = default;
        virtual void Call(size_t thread_index) = 0;
    };

    AlignPtr<ImplBase> impl_;

    template <typename Func>
    struct ImplType final : ImplBase {
	    ImplType(Func&& f)
            : f_(std::forward<Func>(f)) {
        }

        XAMP_ALWAYS_INLINE void Call(size_t thread_index) override {
            f_(thread_index);
        }
        Func f_;
    };
};

using Task = TaskWrapper;

class XAMP_BASE_API XAMP_NO_VTABLE ITaskScheduler {
public:
    XAMP_BASE_CLASS(ITaskScheduler)

	virtual void SubmitJob(Task&& task) = 0;

    virtual void SetAffinityMask(int32_t core) = 0;

    virtual void Destroy() noexcept = 0;
protected:
    ITaskScheduler() = default;
};

// note: 輕量化與並且快速反應的Playback thread.
class XAMP_BASE_API XAMP_NO_VTABLE IThreadPool {
public:
    XAMP_BASE_DISABLE_COPY_AND_MOVE(IThreadPool)

    virtual void Stop() = 0;

    virtual void SetAffinityMask(int32_t core) = 0;

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

#if _MSVC_LANG > 201704L
    using std::bind_front;
#endif
    auto task = MakeAlignedShared<PackagedTaskType>(bind_front(std::forward<F>(f),
        std::forward<Args>(args)...));

    auto future = task->get_future();

    scheduler_->SubmitJob([task](size_t thread_index) mutable {
        (*task)(thread_index);
        });

    return future.share();
}

XAMP_BASE_API AlignPtr<IThreadPool> MakeThreadPool(const std::string_view& pool_name);

XAMP_BASE_API IThreadPool& PlaybackThreadPool();

XAMP_BASE_API IThreadPool& WASAPIThreadPool();

}
