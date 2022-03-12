//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stack>
#include <atomic>
#include <vector>

#include <base/base.h>

namespace xamp::base {

template <typename T>
class LIFOQueue {
public:
    explicit LIFOQueue(size_t size)
        : size_{size} {
    }

    void emplace(T&& item) {
        queue_.emplace(std::move(item));
    }

    T& top() {
        return queue_.top();
    }

    void pop() {
        queue_.pop();
    }

    bool empty() const {
        return queue_.empty();
    }

    size_t size() const {
        return queue_.size();
    }
private:
    size_t size_;
    std::stack<T> queue_;
};

template <typename T>
class WorkStealingQueue {
public:
    struct node_t final {
        T value;
        node_t *next;
    };

    struct head_t final {
        uintptr_t aba;
        node_t *node;

        head_t() noexcept
            : aba(0)
            , node(nullptr) {
        }

        head_t(node_t* ptr) noexcept
            : aba(0)
            , node(ptr) {
        }
    };

    explicit WorkStealingQueue(size_t size)
        : size_{size} {
        head.store(head_t(), std::memory_order_relaxed);
        buffer_.resize(size);
        for (size_t i = 0; i < size - 1; ++i) {
            buffer_[i].next = &buffer_[i + 1];
        }
        buffer_[size-1].next = nullptr;
        free_nodes.store(head_t(buffer_.data()),std::memory_order_relaxed);
    }

    bool TryDequeue(T& task) {
        auto *node = Pop(head);
        if (!node) {
            return false;
        }

        task = std::move(node->value);
        Push(free_nodes, node);
        return true;
    }

    template <typename U>
    bool TryEnqueue(U &&task) {
        auto *node = Pop(free_nodes);
        if (!node) {
            return false;
        }

        node->value = std::move(task);
        Push(head, node);
        return true;
    }

    size_t size() const {
        return 0;
    }

private:
    node_t* Pop(std::atomic<head_t>& h) {
        head_t next, orig = h.load(std::memory_order_relaxed);
        do {
            if (orig.node == nullptr)
                return nullptr;
            next.aba = orig.aba + 1;
            next.node = orig.node->next;
        } while (!h.compare_exchange_weak(orig, next,
                                          std::memory_order_acq_rel,
                                          std::memory_order_acquire));
        return orig.node;
    }

    void Push(std::atomic<head_t>& h, node_t* node) {
        head_t next, orig = h.load(std::memory_order_relaxed);
        do {
            node->next = orig.node;
            next.aba = orig.aba + 1;
            next.node = node;
        } while (!h.compare_exchange_weak(orig, next,
                                          std::memory_order_acq_rel,
                                          std::memory_order_acquire));
    }

    size_t size_;
    std::vector<node_t> buffer_;
    XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<head_t> head;
    XAMP_CACHE_ALIGNED(kCacheAlignSize) std::atomic<head_t> free_nodes;
    uint8_t padding_[kCacheAlignSize - sizeof(free_nodes)]{ 0 };
};

}

