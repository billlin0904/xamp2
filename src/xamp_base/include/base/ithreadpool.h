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
#include <base/bounded_queue.h>

#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
#include <base/logger.h>
#endif

namespace xamp::base {

class TaskWrapper final {
public:
    template <typename Func>
    TaskWrapper(Func&& f)
        : impl_(MakeAlign<ImplBase, ImplType<Func>>(std::forward<Func>(f))) {
    }
	
    XAMP_ALWAYS_INLINE void operator()(int32_t thread_index) {
	    impl_->Call(thread_index);
    }

    XAMP_ALWAYS_INLINE double ExecutedTime() const noexcept {
        return impl_->ExecutedTime();
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
        virtual void Call(int32_t thread_index) = 0;
        [[nodiscard]] virtual double ExecutedTime() const noexcept = 0;
    };

    AlignPtr<ImplBase> impl_;

    template <typename Func>
    struct ImplType final : ImplBase {
	    ImplType(Func&& f)
            : watch()
    		, f_(std::forward<Func>(f)) {
        }

#if 0 // defined(XAMP_ENABLE_THREAD_POOL_DEBUG) && defined(_DEBUG)
        virtual ~ImplType() noexcept override {
            XAMP_LOG_DEBUG("ImplType was deleted.");
        }
#endif

        XAMP_ALWAYS_INLINE void Call(int32_t thread_index) override {
            f_(thread_index);
        }

        [[nodiscard]] double ExecutedTime() const noexcept override {
            return watch.ElapsedSeconds();
        }

        Stopwatch watch;
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

    [[nodiscard]] virtual size_t GetRunningThreadCount() const noexcept = 0;

    [[nodiscard]] virtual size_t GetThreadCount() const noexcept = 0;
protected:
    ITaskScheduler() = default;
};

// note: 輕量化與並且快速反應的Playback thread.
class XAMP_BASE_API XAMP_NO_VTABLE IThreadPool {
public:
    XAMP_BASE_DISABLE_COPY_AND_MOVE(IThreadPool)

	[[nodiscard]] virtual size_t GetRunningThreadCount() const noexcept = 0;

    [[nodiscard]] virtual size_t GetThreadCount() const noexcept = 0;

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
    using ReturnType = std::invoke_result_t<F, int32_t, Args...>;

    // MSVC packaged_task can't be constructed from a move-only lambda
    // https://github.com/microsoft/STL/issues/321
    using PackagedTaskType = std::packaged_task<ReturnType(int32_t)>;

    // std::apply not support std::placeholders
    auto task = MakeAlignedShared<PackagedTaskType>(std::bind(std::forward<F>(f),
        std::placeholders::_1,
        std::forward<Args>(args)...));

    auto future = task->get_future();

    scheduler_->SubmitJob([task](int32_t thread_index) mutable {
        (*task)(thread_index);
        });

    return future.share();
}

XAMP_BASE_API IThreadPool& StreamReaderThreadPool();

XAMP_BASE_API IThreadPool& WASAPIThreadPool();

}
