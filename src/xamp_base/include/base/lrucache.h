//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <list>
#include <optional>

#include <base/base.h>
#include <base/stl.h>

namespace xamp::base {

template <typename Key, typename Value>
class XAMP_BASE_API_ONLY_EXPORT LruCache {
public:
    static const size_t LRU_CACHE_SIZE = 128;

    using ItemList = std::list<std::pair<Key, Value>>;
    using NodePtr = typename ItemList::const_iterator;

    explicit LruCache(size_t max_size = LRU_CACHE_SIZE)
        : max_size_(max_size) {
    }

    void SetMaxSize(size_t max_size) {
        max_size_ = max_size;
    }

    void Insert(const Key& key, const Value& value) {
        items_.emplace_front(key, value);
        cache_[key] = items_.begin();

        if (items_.size() > max_size_) {
            cache_.erase(items_.back().first);
            items_.pop_back();
        }
    }

    std::optional<const Value*> Find(const Key& key) const {
        const auto check = cache_.find(key);
        if (check == cache_.end()) {
            return std::nullopt;
        }
        items_.push_front(*check->second);
        items_.erase(check->second);
        cache_[key] = items_.begin();
        return &check->second->second;
    }

    NodePtr begin() const noexcept {
        return items_.cbegin();
    }

    NodePtr end() const noexcept {
        return items_.cend();
    }

    void Erase(const Key& key) {
        const auto check = cache_.find(key);
        if (check == cache_.end()) {
            return;
        }
        items_.erase(check->second);
        cache_.erase(check);
    }

    void Clear() noexcept {
        cache_.clear();
        items_.clear();
    }

    size_t GetMaxSize() const noexcept {
        return max_size_;
    }

private:
    size_t max_size_;
    mutable RobinHoodHashMap<Key, NodePtr> cache_;
    mutable ItemList items_;
};

}