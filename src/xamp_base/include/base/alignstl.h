//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ctime>
#include <vector>
#include <deque>
#include <string>

#include <base/alignallocator.h>
#include <base/hash.h>
#include <base/id.h>
#include <unordered_map.hpp>

namespace xamp::base {

template <typename T>
using Vector = std::vector<T, AlignedAllocator<T>>;

template <typename T>
using Queue = std::deque<T, AlignedAllocator<T>>;

template <typename T>
struct MurmurHash;

template<> 
struct MurmurHash<std::string> {
	size_t operator()(const std::string& s) const noexcept {
		return MurmurHash64A(s.data(), s.length());
	}
};

template<>
struct MurmurHash<ID> {
	size_t operator()(const ID& id) const noexcept {
		return id.GetHash();
	}
};

template <typename K, typename V, typename H = MurmurHash<K>, typename E = std::equal_to<K>>
using MurmurHashMap = ska::unordered_map<K, V, H, E, AlignedAllocator<std::pair<K, V>>>;

template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>>
using HashMap = ska::unordered_map<K, V, H, E, AlignedAllocator<std::pair<K, V>>>;

template <typename T, typename H = MurmurHash<T>, typename E = std::equal_to<T>>
using Set = ska::unordered_set<T, H, E, AlignedAllocator<T>>;

}

