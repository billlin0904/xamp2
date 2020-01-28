//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <list>
#include <optional>
#include <unordered_map>

template <typename Key, typename Value>
class LruCache {
public:
    static const size_t DEFAULT_CACHE_SIZE = 128;
    using NodePtr = typename std::list<std::pair<Key, Value>>::const_iterator;

    explicit LruCache(size_t max_size = DEFAULT_CACHE_SIZE)
        : max_size_(max_size) {
    }

    void insert(const Key& key, const Value& value) {
        items_list_.emplace_front(key, value);
        items_map_[key] = items_list_.begin();

        if (items_list_.size() > max_size_) {
            items_map_.erase(items_list_.back().first);
            items_list_.pop_back();
        }
    }

    std::optional<const Value*> find(const Key& key) const {
        const auto check = items_map_.find(key);
        if (check == items_map_.end()) {
            return std::nullopt;
        }
        items_list_.push_front(*check->second);
        items_list_.erase(check->second);
        items_map_[key] = items_list_.begin();
        return &check->second->second;
    }

    NodePtr begin() {
        return items_list_.begin();
    }

    NodePtr end() {
        return items_list_.end();
    }

    void erase(const Key& key) {
        const auto check = items_map_.find(key);
        if (check == items_map_.end()) {
            return;
        }
        items_list_.erase(check->second);
        items_map_.erase(check);
    }

    void clear() {
        items_map_.clear();
        items_list_.clear();
    }

    size_t max_size() const noexcept {
        return max_size_;
    }

private:
    size_t max_size_;
    mutable std::unordered_map<Key, NodePtr> items_map_;
    mutable std::list<std::pair<Key, Value>> items_list_;
};
