//=====================================================================================================================
// Copyright (c) 2018-2022 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stack>
#include <atomic>
#include <vector>
#include <optional>

#include <base/base.h>

namespace xamp::base {

//
// Single producer multiple consumer queue.
//
// "Correct and Efficient Work-Stealing for Weak Memory Models
// https://github.com/taskflow/work-stealing-queue/blob/master/references/ppopp13.pdf
//
template
<
    typename T,
    typename V =
    std::enable_if_t
    <
    std::is_nothrow_move_assignable_v<T>
    >
>
class SpmcQueue {
    struct CircularArray {
	    explicit CircularArray(size_t capacity)
	        : capacity_(capacity)
    		, mask_(capacity - 1)
    		, buffer_(new std::atomic<T>[capacity]) {
        }

        ~CircularArray() {
            delete[] buffer_;
        }

        size_t capacity() const noexcept {
            return capacity_;
        }

        template <typename U>
        void push(size_t index, U&& value) noexcept {
            buffer_[index & mask_].store(std::forward<U>(value), std::memory_order_relaxed);
        }

        T pop(size_t index) noexcept {
            return buffer_[index & mask_].load(std::memory_order_relaxed);
        }

        CircularArray* resize(size_t bottom, size_t top) {
            auto* ptr = new CircularArray{ 2 * capacity()};
            for (auto i = top; i != bottom; ++i) {
                ptr->push(i, pop(i));
            }
            return ptr;
        }
    private:
        size_t capacity_;
        size_t mask_;
        std::atomic<T>* buffer_;
    };

public:
    explicit SpmcQueue(size_t size) {
        top_.store(0, std::memory_order_relaxed);
        bottom_.store(0, std::memory_order_relaxed);
        buffer_.store(new CircularArray{ size }, std::memory_order_relaxed);
        garbage_.reserve(32);
    }

    ~SpmcQueue() {
        for (auto a : garbage_) {
            delete a;
        }
        delete buffer_.load();
    }

    template <typename U>
    bool TryEnqueue(U&& item) noexcept {
        Enqueue(std::forward<T>(item));
        return true;
    }

    template <typename U>
    void Enqueue(U&& item) noexcept {
        auto bottom = bottom_.load(std::memory_order_relaxed);
        auto top = top_.load(std::memory_order_acquire);
        auto* a = buffer_.load(std::memory_order_relaxed);

        if (a->capacity() - 1 < (bottom - top)) {
            auto* tmp = a->resize(bottom, top);
            garbage_.push_back(a);
            std::swap(a, tmp);
            buffer_.store(a, std::memory_order_relaxed);
        }

        a->push(bottom, std::forward<U>(item));
        std::atomic_thread_fence(std::memory_order_release);
        bottom_.store(bottom + 1, std::memory_order_relaxed);
    }

    std::optional<T> TrySteal() {
        auto top = top_.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        auto bottom = bottom_.load(std::memory_order_acquire);

        std::optional<T> item;

        if (top < bottom) {
            auto* a = buffer_.load(std::memory_order_consume);
            item = a->pop(top);
            if (!top_.compare_exchange_strong(top, top + 1,
                std::memory_order_seq_cst,
                std::memory_order_relaxed)) {
                return std::nullopt;
            }
        }

        return item;
    }

    std::optional<T> TryDequeue() {
        auto bottom = bottom_.load(std::memory_order_relaxed) - 1;
        auto* a = buffer_.load(std::memory_order_relaxed);
        bottom_.store(bottom, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        auto top = top_.load(std::memory_order_relaxed);

        std::optional<T> item;

        if (top <= bottom) {
            item = a->pop(bottom);
            if (top == bottom) {
                if (!top_.compare_exchange_strong(top, top + 1,
                    std::memory_order_seq_cst,
                    std::memory_order_relaxed)) {
                    item = std::nullopt;
                }
                bottom_.store(bottom + 1, std::memory_order_relaxed);
            }
        }
        else {
            bottom_.store(bottom + 1, std::memory_order_relaxed);
        }

        return item;
    }

    bool IsEmpty() const noexcept {
        const auto bottom = bottom_.load(std::memory_order_relaxed);
        const auto top = top_.load(std::memory_order_relaxed);
        return bottom <= top;
    }

    size_t size() const noexcept {
        return GetSize();
    }

    size_t GetSize() const noexcept {
	    const auto bottom = bottom_.load(std::memory_order_relaxed);
	    const auto top = top_.load(std::memory_order_relaxed);
        return static_cast<size_t>(bottom >= top ? bottom - top : 0);
    }
private:
    std::atomic<size_t> top_;
    std::atomic<size_t> bottom_;
    std::atomic<CircularArray*> buffer_;
    std::vector<CircularArray*> garbage_;
};

}

