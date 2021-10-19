//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <list>
#include <optional>

#include <base/base.h>
#include <base/stl.h>

namespace xamp::base {

inline constexpr size_t kLruCacheSize = 200;
	
template <typename Key, typename Value>
class XAMP_BASE_API_ONLY_EXPORT LruCache {
public:
    using CacheList = std::list<std::pair<Key, Value>>;
    using CacheIterator = typename CacheList::iterator;
    using CacheMap = HashMap<Key, CacheIterator>;

    explicit LruCache(size_t max_size = kLruCacheSize) noexcept;

    void SetMaxSize(size_t max_size);

    void AddOrUpdate(Key const& key, Value const& value);

    Value const* Find(Key const& key) const;

    CacheIterator begin() noexcept {
        return cache_.begin();
    }

    CacheIterator end() noexcept {
        return cache_.end();
    }

    size_t GetMissCount() const noexcept;

    void Erase(Key const& key);

    void Clear() noexcept;

    size_t GetMaxSize() const noexcept;

private:
    size_t max_size_;
    mutable size_t hit_count_;
    mutable size_t miss_count_;
    mutable CacheMap map_;
    mutable CacheList cache_;
};

template <typename Key, typename Value>
LruCache<Key, Value>::LruCache(size_t max_size) noexcept
    : max_size_(max_size)
    , hit_count_(0)
	, miss_count_(0) {
}

template <typename Key, typename Value>
XAMP_ALWAYS_INLINE void LruCache<Key, Value>::SetMaxSize(size_t max_size) {
    max_size_ = max_size;
}

template <typename Key, typename Value>
XAMP_ALWAYS_INLINE void LruCache<Key, Value>::AddOrUpdate(Key const& key, Value const& value) {
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

template <typename Key, typename Value>
XAMP_ALWAYS_INLINE Value const* LruCache<Key, Value>::Find(Key const& key) const {
    const auto check = map_.find(key);
    if (check == map_.end()) {
        ++miss_count_;
        return nullptr;
    }
    ++hit_count_;
    cache_.splice(cache_.begin(), cache_, check->second);
    return &check->second->second;
}

template <typename Key, typename Value>
XAMP_ALWAYS_INLINE size_t LruCache<Key, Value>::GetMissCount() const noexcept {
    return miss_count_;
}

template <typename Key, typename Value>
XAMP_ALWAYS_INLINE void LruCache<Key, Value>::Erase(Key const& key) {
    const auto check = map_.find(key);
    if (check == map_.end()) {
        return;
    }
    cache_.erase(check->second);
    map_.erase(check);
}

template <typename Key, typename Value>
XAMP_ALWAYS_INLINE void LruCache<Key, Value>::Clear() noexcept {
    map_.clear();
    cache_.clear();
    miss_count_ = 0;
}

template <typename Key, typename Value>
XAMP_ALWAYS_INLINE size_t LruCache<Key, Value>::GetMaxSize() const noexcept {
    return max_size_;
}

}
