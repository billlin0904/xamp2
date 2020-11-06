//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>
#include <base/base.h>
#include <base/align_ptr.h>

namespace xamp::base {

template <typename Type>
class XAMP_BASE_API_ONLY_EXPORT SpscQueue {
public:
    using Allocator = std::allocator<Type>;

    SpscQueue(size_t capacity)
        : capacity_(capacity)
        , head_(0)
        , tail_(0) {
        capacity_++;
        if (capacity_ > SIZE_MAX - 2 * kPadding) {
            capacity_ = SIZE_MAX - 2 * kPadding;
        }
        slots_ = std::allocator_traits<Allocator>::allocate(allocator_,
                                                            capacity_ + 2 * kPadding);
        static_assert(alignof(SpscQueue<Type>) == kCacheAlignSize, "");
        static_assert(sizeof(SpscQueue<Type>) >= 3 * kCacheAlignSize, "");
        assert(reinterpret_cast<char *>(&tail_) -
                   reinterpret_cast<char *>(&head_) >=
               static_cast<std::ptrdiff_t>(kCacheAlignSize));
    }

    ~SpscQueue() {
        clear();
        std::allocator_traits<Allocator>::deallocate(allocator_,
                                                     slots_,
                                                     capacity_ + 2 * kPadding);
    }

    void clear() {
        while (Front()) {
            Pop();
        }
    }

    bool empty() const noexcept {
        return size() == 0;
    }

    size_t size() const noexcept {
        std::ptrdiff_t diff = head_.load(std::memory_order_acquire) -
                              tail_.load(std::memory_order_acquire);
        if (diff < 0) {
            diff += capacity_;
        }
        return static_cast<size_t>(diff);
    }

    bool TryPush(const Type &v) {
        return TryEnqueue(v);
    }

    template <typename... Args>
    bool TryEnqueue(Args &&... args) {
        auto const head = head_.load(std::memory_order_relaxed);
        auto nextHead = head + 1;
        if (nextHead == capacity_) {
            nextHead = 0;
        }
        if (nextHead == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        new (&slots_[head + kPadding]) Type(std::forward<Args>(args)...);
        head_.store(nextHead, std::memory_order_release);
        return true;
    }

    Type *Front() noexcept {
        auto const tail = tail_.load(std::memory_order_relaxed);
        if (head_.load(std::memory_order_acquire) == tail) {
            return nullptr;
        }
        return &slots_[tail + kPadding];
    }

    void Pop() noexcept {
        auto const tail = tail_.load(std::memory_order_relaxed);
        assert(head_.load(std::memory_order_acquire) != tail);
        slots_[tail + kPadding].~Type();
        auto nextTail = tail + 1;
        if (nextTail == capacity_) {
            nextTail = 0;
        }
        tail_.store(nextTail, std::memory_order_release);
    }

    XAMP_DISABLE_COPY(SpscQueue)

private:
    static constexpr size_t kPadding = (kCacheAlignSize - 1) / sizeof(Type) + 1;

    size_t capacity_;
    Type *slots_;
    Allocator allocator_;

    XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<size_t> head_;
    XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<size_t> tail_;

    uint8_t padding_[kCacheAlignSize - sizeof(tail_)]{ 0 };
};

}
