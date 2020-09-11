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

static constexpr size_t kLruCacheSize = 200;
	
template <typename Key, typename Value>
class XAMP_BASE_API_ONLY_EXPORT LruCache {
public:    
    using CacheList = std::list<std::pair<Key, Value>>;
    using CachePtr = typename CacheList::iterator;

    explicit LruCache(size_t max_size = kLruCacheSize)
        : max_size_(max_size)
        , miss_count_(0) {
    }

    void SetMaxSize(size_t max_size) {
        max_size_ = max_size;
    }

    void Insert(Key const& key, Value const& value) {
        auto check = map_.find(key);
        if (check != map_.cend()) {
            check->second->second = value;
            cache_.splice(cache_.begin(), cache_, check->second);
        }
        else {
            if (cache_.size() == max_size_) {
                map_.erase(cache_.back().first);
                cache_.pop_back();
            }
            cache_.emplace_front(key, value);
            map_[key] = cache_.begin();
        }
    }

    std::optional<Value const*> Find(Key const& key) const {
        const auto check = map_.find(key);
        if (check == map_.end()) {
            ++miss_count_;
            return std::nullopt;
        }
        cache_.splice(cache_.begin(), cache_, check->second);
        return &check->second->second;
    }

    CachePtr begin() noexcept {
        return cache_.begin();
    }

    CachePtr end() noexcept {
        return cache_.end();
    }

    size_t GetMissCount() const noexcept {
        return miss_count_;
    }

    void Erase(Key const& key) {
        const auto check = map_.find(key);
        if (check == map_.end()) {
            return;
        }
        cache_.erase(check->second);
        map_.erase(check);
    }

    void Clear() noexcept {
        map_.clear();
        cache_.clear();
        miss_count_ = 0;
    }

    size_t GetMaxSize() const noexcept {
        return max_size_;
    }

private:
    size_t max_size_;
    mutable size_t miss_count_;
    mutable RobinHoodHashMap<Key, CachePtr> map_;
    mutable CacheList cache_;
};

}