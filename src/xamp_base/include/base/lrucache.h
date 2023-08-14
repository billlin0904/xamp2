//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
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
struct XAMP_BASE_API_ONLY_EXPORT DefaultSizeOfPolicy {
	int64_t operator()(const Key &, const Value &) {
        return 1;
	}
};

template
<
    typename Key,
	typename Value,
    typename SizeOfPolicy = DefaultSizeOfPolicy<Key, Value>,
	typename KeyList = List<std::pair<Key, Value>>,
    typename SharedMutex = std::shared_mutex
>
class XAMP_BASE_API_ONLY_EXPORT LruCache {
public:
    using KeyIterator = typename KeyList::iterator;
    using CacheMap = HashMap<Key, KeyIterator>;

    explicit LruCache(int64_t capacity = kLruCacheSize) noexcept;

    void Resize(int64_t capacity);

    void AddOrUpdate(Key const& key, Value value);

    Value GetOrAdd(Key const& key, std::function<Value()> &&value_factory);

    bool Add(Key const& key, Value value);

    int64_t GetMissCount() const noexcept;

    int64_t GetHitCount() const noexcept;

    void Erase(Key const& key);

    void Clear() noexcept;

    int64_t GetSize() const noexcept;

    int64_t GetMaxSize() const noexcept;

    KeyIterator begin() noexcept {
        return keys_.begin();
    }

    KeyIterator end() noexcept {
        return keys_.end();
    }

    void Evict(int64_t max_size);

    bool IsFulled(size_t new_entry_size) const noexcept {
        std::shared_lock<SharedMutex> read_lock{ mutex_ };
        return size_ + new_entry_size >= capacity_;
    }

    bool Contains(Key const& key) const noexcept {
        std::shared_lock<SharedMutex> read_lock{ mutex_ };
        return cache_.contains(key);
	}
private:
    friend std::ostream& operator<< (std::ostream& ostr, const LruCache& cache) {
        auto hit_count = cache.GetHitCount();
        auto max_size = cache.GetMaxSize();
        auto size = cache.GetSize();
        auto miss_count = cache.GetMissCount();
        auto accesses = hit_count + miss_count;
        auto hit_percent = accesses != 0 ? (100 * hit_count / accesses) : 0;
        ostr << "LruCache[size=" << size << ",hits=" << hit_count << ",miss=" << miss_count << ",hit-rate=" << hit_percent << "%]";
        return ostr;
    }

    int64_t size_;
    int64_t capacity_;
    mutable size_t hit_count_;
    mutable size_t miss_count_;
    mutable CacheMap cache_;
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
LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::LruCache(int64_t capacity) noexcept
    : size_(0)
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
void LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::Resize(int64_t capacity) {
    {
        std::unique_lock<SharedMutex> write_lock(mutex_);
        capacity_ = capacity;
    }
    Evict(capacity_);
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
bool LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::Add(Key const& key, Value value) {
    {
        std::unique_lock<SharedMutex> write_lock(mutex_);

        if (cache_.count(key)) {
            return false;
        }

        size_ += policy_(key, value);
        keys_.emplace_front(key, std::move(value));
        cache_[key] = keys_.begin();
    }

    Evict(capacity_);
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
Value LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::GetOrAdd(Key const& key, std::function<Value()>&& value_factory) {
    {
        std::unique_lock<SharedMutex> write_lock(mutex_);
        const auto check = cache_.find(key);
        if (check != cache_.end()) {
            ++hit_count_;
            keys_.splice(keys_.begin(), keys_, check->second);
            return check->second->second;
        }
        ++miss_count_;
    }

    auto value = value_factory();
  
    {
        std::unique_lock<SharedMutex> write_lock(mutex_);
        size_ += policy_(key, value);
        keys_.emplace_front(key, value);
        cache_[key] = keys_.begin();
    }

    Evict(capacity_);

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
void LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::AddOrUpdate(Key const& key, Value value) {
    {
        std::unique_lock<SharedMutex> write_lock(mutex_);
        size_ += policy_(key, value);
        auto itr = cache_.find(key);
        if (itr != cache_.cend()) {
            size_ -= policy_(key, value);
            itr->second->second = std::move(value);
            keys_.splice(keys_.begin(), keys_, itr->second);
        } else {
            keys_.emplace_front(key, std::move(value));
            cache_[key] = keys_.begin();
        }
    }

    Evict(capacity_);
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
void LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::Evict(int64_t max_size) {
    while (true) {
        std::unique_lock<SharedMutex> write_lock(mutex_);
        if (!size_ || size_ <= max_size) {
            break;
        }
        auto& eldest = keys_.back();
        size_ -= policy_(eldest.first, eldest.second);
        cache_.erase(eldest.first);
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
int64_t LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::GetMissCount() const noexcept {
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
int64_t LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::GetHitCount() const noexcept {
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
void LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::Erase(Key const& key) {
    std::unique_lock<SharedMutex> write_lock(mutex_);

    const auto check = cache_.find(key);
    if (check == cache_.end()) {
        return;
    }

    size_ -= policy_(check->first, check->second->second);
    keys_.erase(check->second);
    cache_.erase(check);
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
void LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::Clear() noexcept {
    Evict(-1);
}

template
<
    typename Key,
    typename Value,
    typename SizeOfPolicy,
    typename KeyList,
    typename SharedMutex
>
int64_t LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::GetMaxSize() const noexcept {
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
int64_t LruCache<Key, Value, SizeOfPolicy, KeyList, SharedMutex>::GetSize() const noexcept {
    std::shared_lock<SharedMutex> read_lock{ mutex_ };
    return size_;
}

XAMP_BASE_NAMESPACE_END