//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
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

#if __cplusplus >= XAMP_CPP20_LANG_VER
    using std::bind_front;
#endif
    auto task = MakeAlignedShared<PackagedTaskType>(bind_front(std::forward<F>(f),
        std::forward<Args>(args)...));

    auto future = task->get_future();

    scheduler_->SubmitJob([task](size_t thread_index) {
        (*task)(thread_index);
        });

    return future.share();
}

XAMP_BASE_API AlignPtr<IThreadPool> MakeThreadPool(const std::string_view& pool_name);

XAMP_BASE_API IThreadPool& PlaybackThreadPool();

XAMP_BASE_API IThreadPool& WASAPIThreadPool();

template <typename C, typename Func>
void ParallelFor(C& items, Func&& f, IThreadPool& tp, size_t batches = 4) {
    auto begin = std::begin(items);
    auto size = std::distance(begin, std::end(items));

    std::vector<std::shared_future<void>> futures(batches);
    size_t i = 0;
    for (auto& ff : futures) {
        ff = tp.Spawn([f, begin, i](size_t) -> void {
            f(*(begin + i));
            });
        ++i;
    }

    for (auto& ff : futures) {
        ff.wait();
    }
}

template <typename Func>
void ParallelFor(size_t begin, size_t end, Func &&f, IThreadPool& tp, size_t batches = 4) {
    std::vector<std::shared_future<void>> futures(batches);
    const auto block_iters = 
        std::ceil(static_cast<float>(end - begin)
            / static_cast<float>(batches));

    auto block_begin = begin - block_iters;
    auto block_end = begin;
    auto step = [&]() -> void
    {
        block_begin += block_iters;
        block_end += block_iters;
        block_end = (block_end > end) ? end : block_end;
    };

    step();

    for (auto& ff : futures) {
        ff = tp.Spawn([=, &f](size_t) -> void {
                f(block_begin, block_end);
            });
        step();
    }

    for (auto& ff : futures) {
        ff.wait();
    }
}

}
