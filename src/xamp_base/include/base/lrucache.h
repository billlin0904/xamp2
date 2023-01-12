//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/stl.h>

#include <shared_mutex>

namespace xamp::base {

inline constexpr size_t kLruCacheSize = 200;
	
template
<
    typename Key,
	typename Value,
	typename KeyList = List<std::pair<Key, Value>>,
    typename SharedMutex = std::shared_mutex
>
class XAMP_BASE_API_ONLY_EXPORT LruCache {
public:
    using KeyIterator = typename KeyList::iterator;
    using CacheMap = HashMap<Key, KeyIterator>;

    explicit LruCache(size_t max_size = kLruCacheSize) noexcept;

    void SetMaxSize(size_t max_size);

    void AddOrUpdate(Key const& key, Value value);

    bool Add(Key const& key, Value value);

    Value const* Find(Key const& key) const;

    KeyIterator begin() noexcept {
        return keys_.begin();
    }

    KeyIterator end() noexcept {
        return keys_.end();
    }

    size_t GetMissCount() const noexcept;

    size_t GetHitCount() const noexcept;

    void Erase(Key const& key);

    void Clear() noexcept;

    size_t GetMaxSize() const noexcept;

private:
    void Evict();

    size_t max_size_;
    mutable size_t hit_count_;
    mutable size_t miss_count_;
    mutable CacheMap cache_;
    mutable KeyList keys_;
    mutable SharedMutex mutex_;
};

template <typename Key, typename Value, typename KeyList, typename SharedMutex>
LruCache<Key, Value, KeyList, SharedMutex>::LruCache(size_t max_size) noexcept
    : max_size_(max_size)
    , hit_count_(0)
	, miss_count_(0) {
}

template <typename Key, typename Value, typename KeyList, typename SharedMutex>
XAMP_ALWAYS_INLINE void LruCache<Key, Value, KeyList, SharedMutex>::SetMaxSize(size_t max_size) {
    std::unique_lock<SharedMutex> write_lock(mutex_);
    max_size_ = max_size;
}

template <typename Key, typename Value, typename KeyList, typename SharedMutex>
XAMP_ALWAYS_INLINE bool LruCache<Key, Value, KeyList, SharedMutex>::Add(Key const& key, Value value) {
    std::unique_lock<SharedMutex> write_lock(mutex_);

    if (cache_.count(key)) {
        return false;
    }

    Evict();

	keys_.emplace_front(key, std::move(value));
    cache_[key] = keys_.begin();
    return true;
}

template <typename Key, typename Value, typename KeyList, typename SharedMutex>
XAMP_ALWAYS_INLINE void LruCache<Key, Value, KeyList, SharedMutex>::AddOrUpdate(Key const& key, Value value) {
    std::unique_lock<SharedMutex> write_lock(mutex_);

    auto itr = cache_.find(key);

    if (itr != cache_.cend()) {
        itr->second->second = std::move(value);
        keys_.splice(keys_.begin(), keys_, itr->second);
    }
    else {
        Evict();
        keys_.emplace_front(key, std::move(value));
        cache_[key] = keys_.begin();
    }
}

template <typename Key, typename Value, typename KeyList, typename SharedMutex>
XAMP_ALWAYS_INLINE void LruCache<Key, Value, KeyList, SharedMutex>::Evict() {
    while (keys_.size() > max_size_) {
        cache_.erase(keys_.back().first);
        keys_.pop_back();
    }
}

template <typename Key, typename Value, typename KeyList, typename SharedMutex>
XAMP_ALWAYS_INLINE Value const* LruCache<Key, Value, KeyList, SharedMutex>::Find(Key const& key) const {
    std::shared_lock<SharedMutex> read_lock{ mutex_ };

    const auto check = cache_.find(key);
    if (check == cache_.end()) {
        ++miss_count_;
        return nullptr;
    }
    ++hit_count_;
    keys_.splice(keys_.begin(), keys_, check->second);
    return &check->second->second;
}

template <typename Key, typename Value, typename KeyList, typename SharedMutex>
XAMP_ALWAYS_INLINE size_t LruCache<Key, Value, KeyList, SharedMutex>::GetMissCount() const noexcept {
    std::shared_lock<SharedMutex> read_lock{ mutex_ };
    return miss_count_;
}

template <typename Key, typename Value, typename KeyList, typename SharedMutex>
XAMP_ALWAYS_INLINE size_t LruCache<Key, Value, KeyList, SharedMutex>::GetHitCount() const noexcept {
    std::shared_lock<SharedMutex> read_lock{ mutex_ };
    return hit_count_;
}

template <typename Key, typename Value, typename KeyList, typename SharedMutex>
XAMP_ALWAYS_INLINE void LruCache<Key, Value, KeyList, SharedMutex>::Erase(Key const& key) {
    std::unique_lock<SharedMutex> write_lock(mutex_);

    const auto check = cache_.find(key);
    if (check == cache_.end()) {
        return;
    }
    keys_.erase(check->second);
    cache_.erase(check);
}

template <typename Key, typename Value, typename KeyList, typename SharedMutex>
XAMP_ALWAYS_INLINE void LruCache<Key, Value, KeyList, SharedMutex>::Clear() noexcept {
    std::shared_lock<SharedMutex> read_lock{ mutex_ };
    cache_.clear();
    keys_.clear();
    miss_count_ = 0;
}

template <typename Key, typename Value, typename KeyList, typename SharedMutex>
XAMP_ALWAYS_INLINE size_t LruCache<Key, Value, KeyList, SharedMutex>::GetMaxSize() const noexcept {
    std::shared_lock<SharedMutex> read_lock{ mutex_ };
    return max_size_;
}

}
