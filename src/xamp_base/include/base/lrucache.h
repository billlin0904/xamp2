//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <list>
#include <optional>

#include <base/alignstl.h>

#include <base/base.h>

namespace xamp::base {

template <typename Key, typename Value>
class XAMP_BASE_API_ONLY_EXPORT LruCache {
public:
    static const size_t DEFAULT_CACHE_SIZE = 128;

    using List = std::list<std::pair<Key, Value>, AlignedAllocator<std::pair<Key, Value>>>;
    using NodePtr = typename List::const_iterator;

    explicit LruCache(size_t max_size = DEFAULT_CACHE_SIZE)
        : max_size_(max_size) {
    }

    void set_cache_size(size_t max_size) {
        max_size_ = max_size;
    }

    void insert(const Key& key, const Value& value) {
        cache_.emplace_front(key, value);
        items_[key] = cache_.begin();

        if (cache_.size() > max_size_) {
            items_.erase(cache_.back().first);
            cache_.pop_back();
        }
    }

    std::optional<const Value*> find(const Key& key) const {
        const auto check = items_.find(key);
        if (check == items_.end()) {
            return std::nullopt;
        }
        cache_.push_front(*check->second);
        cache_.erase(check->second);
        items_[key] = cache_.begin();
        return &check->second->second;
    }

    NodePtr begin() const noexcept {
        return cache_.cbegin();
    }

    NodePtr end() const noexcept {
        return cache_.cend();
    }

    void erase(const Key& key) {
        const auto check = items_.find(key);
        if (check == items_.end()) {
            return;
        }
        cache_.erase(check->second);
        items_.erase(check);
    }

    void clear() {
        items_.clear();
        cache_.clear();
    }

    size_t max_size() const noexcept {
        return max_size_;
    }

private:
    size_t max_size_;
    mutable HashMap<Key, NodePtr> items_;
    mutable List cache_;
};

}