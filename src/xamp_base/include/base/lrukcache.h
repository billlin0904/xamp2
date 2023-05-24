//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/stl.h>
#include <base/str_utilts.h>

#include <functional>
#include <shared_mutex>

XAMP_BASE_NAMESPACE_BEGIN

template
<
    typename Key,
    typename Value,
    typename SharedMutex = std::shared_mutex
>
class XAMP_BASE_API_ONLY_EXPORT LruKCache {
public:   
    struct Node {
        Key key;
        Value value;
        size_t visited{ 0 };
    };

    explicit LruKCache(int64_t k = 2, int64_t size = 2, int64_t history_size = 4) noexcept;

    void AddOrUpdate(const Key& key, const Value& value);

    std::optional<Value> Get(const Key& key);

    int64_t GetMissCount() const noexcept;

    int64_t GetHitCount() const noexcept;

    void Clear();

private:
    friend std::ostream& operator<< (std::ostream& ostr, const LruKCache& cache) {
        auto hit_count = cache.GetHitCount();
        auto max_size = cache.GetCapacity();
        auto miss_count = cache.GetMissCount();
        auto accesses = hit_count + miss_count;
        auto hit_percent = accesses != 0 ? (100 * hit_count / accesses) : 0;
        ostr << "LruKCache hits=" << hit_count << ",miss=" << miss_count << ",hit-rate=" << hit_percent << "%]";
        return ostr;
    }

    using NodeIterator = typename std::list<Node>::iterator;

    void MoveToFront(std::list<Node>& lst, typename std::list<Node>::iterator iter) {
        if (iter != lst.begin()) {
            lst.splice(lst.begin(), lst, iter);
        }
    }

    HashMap<Key, NodeIterator>::iterator AddHistory(const Node& node) {
        if (history_size_ == 0) {
            ++history_size_;
            auto &node = history_queue_.back();
            history_.erase(node.key);
            history_queue_.pop_back();
        }
        --history_size_;
        history_queue_.push_front(node);
        history_[node.key] = history_queue_.begin();
        return history_.find(node.key);
    }

    void RemoveHistoryElement(typename std::list<Node>::iterator iter) {
        ++history_size_;
        history_.erase((*iter).key);
        history_queue_.erase(iter);
    }

    void AddElement(const Node &node) {
        bool evicted = false;
        if (cache_size_ == 0) {
            evicted = true;
            ++cache_size_;
            auto& node = cache_queue_.back();
            cache_.erase(node.key);
            cache_queue_.pop_back();
        }
        --cache_size_;
        cache_queue_.push_front(node);
        cache_[node.key] = cache_queue_.begin();
    }

    int64_t k_;    
    int64_t cache_size_;
    int64_t history_size_;
    mutable size_t hit_count_;
    mutable size_t miss_count_;
    std::list<Node> history_queue_;
    std::list<Node> cache_queue_;
    HashMap<Key, NodeIterator> cache_;
    HashMap<Key, NodeIterator> history_;
    mutable SharedMutex mutex_;
};

template
<
    typename Key,
    typename Value,
    typename SharedMutex
>
LruKCache<Key, Value, SharedMutex>::LruKCache(int64_t k, int64_t size, int64_t history_size) noexcept
    : k_(k)
    , history_size_(history_size)
    , cache_size_(size)
    , hit_count_(0)
    , miss_count_(0) {
}

template
<
    typename Key,
    typename Value,
    typename SharedMutex
>
void LruKCache<Key, Value, SharedMutex>::AddOrUpdate(const Key& key, const Value& value) {
    std::unique_lock<SharedMutex> write_lock(mutex_);
    auto itr = cache_.find(key);
    if (itr != cache_.end()) {
        (*itr).second->value = value;
        MoveToFront(cache_queue_, (*itr).second);
        return;
    }

    itr = history_.find(key);
    if (itr != history_.end()) {
        (*itr).second->visited++;
        if ((*itr).second->visited >= k_) {
            RemoveHistoryElement((*itr).second);
            AddElement(Node{ key, value, 1 });
            return;
        }
        MoveToFront(history_queue_, (*itr).second);
    }
    else {
        itr = AddHistory(Node{ key, value, 1 });
    }
}

template
<
    typename Key,
    typename Value,
    typename SharedMutex
>
std::optional<Value> LruKCache<Key, Value, SharedMutex>::Get(const Key& key) {
    std::unique_lock<SharedMutex> write_lock(mutex_);
    auto itr = cache_.find(key);
    if (itr != cache_.end()) {
        MoveToFront(cache_queue_, (*itr).second);
        return (*itr).second->value;
    }
    return std::nullopt;
}

template
<
    typename Key,
    typename Value,
    typename SharedMutex
>
int64_t LruKCache<Key, Value, SharedMutex>::GetHitCount() const noexcept {
    std::shared_lock<SharedMutex> read_lock{ mutex_ };
    return hit_count_;
}

template
<
    typename Key,
    typename Value,
    typename SharedMutex
>
int64_t LruKCache<Key, Value, SharedMutex>::GetMissCount() const noexcept {
    std::shared_lock<SharedMutex> read_lock{ mutex_ };
    return miss_count_;
}

template
<
    typename Key,
    typename Value,
    typename SharedMutex
>
void LruKCache<Key, Value, SharedMutex>::Clear() {
    std::unique_lock<SharedMutex> write_lock(mutex_);
    cache_.clear();
    history_queue_.clear();
    cache_queue_.clear();
}

XAMP_BASE_NAMESPACE_END