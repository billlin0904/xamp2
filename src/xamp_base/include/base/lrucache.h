//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/stl.h>

#include <functional>
#include <shared_mutex>

XAMP_BASE_NAMESPACE_BEGIN

inline constexpr int64_t kLruCacheSize = 200;

template
<
    typename Key,
    typename Value
>
struct DefaultSizeOfPolicy {
	int64_t operator()(const Key &, const Value &) {
        return 1;
	}
};

template
<
    typename Key,
	typename Value,
    typename SizeOfPolicy = DefaultSizeOfPolicy<Key, Value>,
	typename KeyList = std::list<std::pair<Key, Value>>,
    typename SharedMutex = std::shared_mutex
>
class LruCache {
public:
	XAMP_DECLARE_SINGLETON_NAME()

    using KeyIterator = typename KeyList::iterator;
    using CacheMap = HashMap<Key, KeyIterator>;

    explicit LruCache(int64_t capacity = kLruCacheSize) ;

    void resize(int64_t capacity);

    bool tryGet(Key const& key, Value &value);

    void addOrUpdate(Key const& key, Value value);

    Value getOrAdd(Key const& key, std::move_only_function<Value()> &&value_factory);

    bool add(Key const& key, Value value);

    int64_t getMissCount() const ;

    int64_t getHitCount() const ;

    void erase(Key const& key);

    void clear() ;

    int64_t getSize() const ;

    int64_t getMaxSize() const ;

    void evict(int64_t max_size);

    bool isFull(int64_t new_entry_size) const {
        std::shared_lock<SharedMutex> read_lock{ mutex_ };
        return size_ + new_entry_size > capacity_;
    }

    bool contains(Key const& key) const {
        std::shared_lock<SharedMutex> read_lock{ mutex_ };
        return thumbnail_cache_.find(key) != thumbnail_cache_.end();
	}
private:
    friend std::ostream& operator<< (std::ostream& ostr, const LruCache& cache) {
        auto hit_count = cache.getHitCount();
        auto max_size = cache.getMaxSize();
        auto size = cache.getSize();
        auto miss_count = cache.getMissCount();
        auto accesses = hit_count + miss_count;
        auto hit_percent = accesses != 0 ? (100 * hit_count / accesses) : 0;
        ostr << "[size=" << size << ",hits=" << hit_count << ",miss=" << miss_count << ",hit-rate=" << hit_percent << "%]";
        return ostr;
    }

    void evictLocked(int64_t max_size);

    int64_t size_;
    int64_t capacity_;
    mutable size_t hit_count_;
    mutable size_t miss_count_;
    mutable CacheMap thumbnail_cache_;
    mutable KeyList keys_;
    mutable SharedMutex mutex_;
    SizeOfPolicy policy_;
};

template
<
    typename Key,
	typename Value,
	typename SizeOfPolicy,
	typename KeyList,
	typename SharedMutex
>
LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::LruCache(int64_t capacity) : size_(0)
	, capacity_(capacity)
    , hit_count_(0)
	, miss_count_(0) {
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
void LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::resize(int64_t capacity) {
    std::unique_lock<SharedMutex> write_lock(mutex_);
    capacity_ = capacity;
    evictLocked(capacity_);
}

template<typename Key, typename Value, typename SizeOfPolicy, typename KeyList, typename SharedMutex>
bool LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::tryGet(Key const& key, Value& value) {
    std::unique_lock<SharedMutex> write_lock(mutex_);
    const auto check = thumbnail_cache_.find(key);
    if (check != thumbnail_cache_.end()) {
        ++hit_count_;
        keys_.splice(keys_.begin(), keys_, check->second);
        value = check->second->second;
        return true;
    }
    ++miss_count_;
    return false;
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
bool LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::add(Key const& key, Value value) {
    std::unique_lock<SharedMutex> write_lock(mutex_);

    if (thumbnail_cache_.contains(key)) {
        return false;
    }

    size_ += policy_(key, value);
    keys_.emplace_front(key, std::move(value));
    thumbnail_cache_[key] = keys_.begin();
    evictLocked(capacity_);
    return true;
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
Value LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::getOrAdd(Key const& key, std::move_only_function<Value()>&& value_factory) {
    {
        std::unique_lock<SharedMutex> write_lock(mutex_);
        const auto check = thumbnail_cache_.find(key);
        if (check != thumbnail_cache_.end()) {
            ++hit_count_;
            keys_.splice(keys_.begin(), keys_, check->second);
            return check->second->second;
        }
        ++miss_count_;
    }

    auto value = value_factory();
  
    {
        std::unique_lock<SharedMutex> write_lock(mutex_);
        const auto check = thumbnail_cache_.find(key);
        if (check != thumbnail_cache_.end()) {
            // 已經存在，直接返回該值 (並更新 LRU)
            ++hit_count_;
            keys_.splice(keys_.begin(), keys_, check->second);
            return check->second->second;
        }

        size_ += policy_(key, value);
        keys_.emplace_front(key, value);
        thumbnail_cache_[key] = keys_.begin();

        evictLocked(capacity_);
    }    

    return value;
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
void LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::addOrUpdate(Key const& key, Value value) {
    std::unique_lock<SharedMutex> write_lock(mutex_);

    auto itr = thumbnail_cache_.find(key);
    if (itr != thumbnail_cache_.cend()) {
        size_ -= policy_(key, itr->second->second);
        size_ += policy_(key, value);
        itr->second->second = std::move(value);
        keys_.splice(keys_.begin(), keys_, itr->second);
    }
    else {
        size_ += policy_(key, value);
        keys_.emplace_front(key, std::move(value));
        thumbnail_cache_[key] = keys_.begin();
    }

    evictLocked(capacity_);
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
void LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::evict(int64_t max_size) {
    std::unique_lock<SharedMutex> write_lock(mutex_);
    evictLocked(max_size);
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
void LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::evictLocked(int64_t max_size) {
    while (size_ > max_size && !keys_.empty()) {
        auto& eldest = keys_.back();
        size_ -= policy_(eldest.first, eldest.second);
        thumbnail_cache_.erase(eldest.first);
        keys_.pop_back();
    }
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
int64_t LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::getMissCount() const {
    std::shared_lock<SharedMutex> read_lock{ mutex_ };
    return miss_count_;
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
int64_t LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::getHitCount() const {
    std::shared_lock<SharedMutex> read_lock{ mutex_ };
    return hit_count_;
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
void LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::erase(Key const& key) {
    std::unique_lock<SharedMutex> write_lock(mutex_);

    const auto check = thumbnail_cache_.find(key);
    if (check == thumbnail_cache_.end()) {
        return;
    }

    size_ -= policy_(check->first, check->second->second);
    keys_.erase(check->second);
    thumbnail_cache_.erase(check);
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
void LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::clear() {
    std::unique_lock<SharedMutex> write_lock(mutex_);
    size_ = 0;
    keys_.clear();
    thumbnail_cache_.clear();
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
int64_t LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::getMaxSize() const {
    std::shared_lock<SharedMutex> read_lock{ mutex_ };
    return capacity_;
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
int64_t LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::getSize() const {
    std::shared_lock<SharedMutex> read_lock{ mutex_ };
    return size_;
}

XAMP_BASE_NAMESPACE_END
